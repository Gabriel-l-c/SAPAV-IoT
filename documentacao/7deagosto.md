# Documentação de Integração: Radar LD2420 + ESP8266 + FIWARE Orion

## 🎯 O Sucesso Alcançado
Foi estabelecida uma ponte de comunicação em tempo real entre o hardware físico e o banco de dados IoT. O fluxo validado é:
1. **ESP8266** lê o sensor LD2420 e envia via USB (`115200` baud).
2. **Raspberry Pi** lê a porta serial, extrai a distância usando Expressões Regulares (Regex) e converte para dados estruturados.
3. **FIWARE Orion (192.168.0.95)** recebe e armazena os dados via requisições HTTP REST (`PATCH`) no padrão NGSI-v2.

---

## 🛠️ O Caminho e as Soluções (Troubleshooting)

Para chegar ao funcionamento perfeito, foram resolvidos 4 gargalos principais:

1. **Permissão da Porta USB (`[Errno 13] Permission denied`):**
   * **Problema:** O Linux bloqueava a leitura do usuário comum na porta `/dev/ttyUSB0`.
   * **Solução:** Liberação manual de leitura e escrita.
   * **Comando:** `sudo chmod 666 /dev/ttyUSB0`

2. **Ruído de Leitura e Incompatibilidade de Baud Rate:**
   * **Problema:** Erros de decodificação UART porque o Python lia a 9600 baud, mas o ESP8266 enviava a 115200 baud.
   * **Solução:** Atualização da variável `BAUD_RATE` para `115200` e uso do parâmetro `errors='replace'` para filtrar ruídos elétricos na decodificação.
   
3. **Formatação Estrita do FIWARE (Erro 400 BadRequest):**
   * **Problema:** O FIWARE rejeitava a criação da entidade porque o campo `metadata` não era um objeto JSON completo. O FIWARE também recusava requisições `GET` 
que contivessem o cabeçalho `Content-Type`.
   * **Solução:** O payload foi reescrito para que todos os metadados contivessem obrigatoriamente `type` e `value`. O cabeçalho foi removido da função de *Health Check*.

4. **Paradoxo de Entidade (Erros 404 e 422 - Tenants):**
   * **Problema:** O script não encontrava o sensor (Status 404), mas falhava ao tentar criá-lo informando que ele já existia (Status 422).
   * **Solução:** O erro ocorria devido ao uso inconsistente do cabeçalho de inquilino (`"Fiware-Service": "default"`). A solução foi limpar o banco de dados via terminal
 e remover os cabeçalhos de *Tenant* do código Python, utilizando o escopo público padrão.
   * **Comandos de Limpeza Utilizados:**
     ```bash
     curl -X DELETE "[http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001?type=SensorUart](http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001?type=SensorUart)"
     curl -X DELETE "[http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001?type=SensorUart](http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001?type=SensorUart)" -H "Fiware-Service: default" -H "Fiware-ServicePath: /"
     ```

---

## 💻 Código Final Consolidado (Script Python)

O código abaixo é o script definitivo validado. 

**Arquivo:** `fiware_sensor_integration.py`

```python
#!/usr/bin/env python3
import serial
import requests
import time
import logging
import sys
import re
from datetime import datetime
from typing import Optional, Dict, Any

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s', handlers=[logging.StreamHandler(sys.stdout)])
logger = logging.getLogger(__name__)

class Config:
    BROKER_URL = "[http://192.168.0.95:31330](http://192.168.0.95:31330)"
    ENTITY_ID = "urn:ngsi-ld:SensorUart:001"
    ENTITY_TYPE = "SensorUart"
    SERIAL_PORT = "/dev/ttyUSB0"
    BAUD_RATE = 115200
    TIMEOUT = 1
    MAX_RETRIES = 3
    SEND_INTERVAL = 5

class FiwareOrionClient:
    def __init__(self, broker_url: str, entity_id: str, entity_type: str):
        self.broker_url, self.entity_id, self.entity_type = broker_url, entity_id, entity_type
        self.headers = {"Content-Type": "application/json"}
    
    def _make_request(self, method: str, endpoint: str, data: Optional[Dict] = None) -> Optional[Dict]:
        url = f"{self.broker_url}{endpoint}"
        try:
            if method == "GET": resp = requests.get(url, timeout=5)
            elif method == "POST": resp = requests.post(url, headers=self.headers, json=data, timeout=5)
            elif method == "PATCH": resp = requests.patch(url, headers=self.headers, json=data, timeout=5)
            if resp.status_code in [200, 201, 204]: return resp.json() if resp.text else {}
            return None
        except: return None
    
    def check_broker_health(self) -> bool: return bool(self._make_request("GET", "/version"))
    def entity_exists(self) -> bool: return bool(self._make_request("GET", f"/v2/entities/{self.entity_id}"))
    
    def create_entity(self) -> bool:
        payload = {
            "id": self.entity_id, "type": self.entity_type,
            "presenca": {"value": False, "type": "Boolean", "metadata": {"description": {"type": "Text", "value": "Presença"}}},
            "distancia": {"value": 0, "type": "Integer", "metadata": {"unitCode": {"type": "Text", "value": "CMT"}}},
            "timestamp": {"value": datetime.now().isoformat(), "type": "DateTime"}
        }
        return bool(self._make_request("POST", "/v2/entities", payload))
    
    def update_sensor_data(self, presenca: bool, distancia: int) -> bool:
        logger.info(f"Atualizando: presenca={presenca}, distancia={distancia}cm")
        payload = {
            "presenca": {"value": presenca, "type": "Boolean"},
            "distancia": {"value": distancia, "type": "Integer"},
            "timestamp": {"value": datetime.now().isoformat(), "type": "DateTime"}
        }
        return bool(self._make_request("PATCH", f"/v2/entities/{self.entity_id}/attrs", payload))

class SensorUart:
    def __init__(self, port: str, baudrate: int):
        self.port, self.baudrate, self.ser = port, baudrate, None
    
    def connect(self) -> bool:
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)
            return True
        except: return False
    
    def read_data(self) -> Optional[str]:
        try:
            if self.ser and self.ser.in_waiting > 0:
                return self.ser.readline().decode('utf-8', errors='replace').strip()
        except: pass
        return None
    
    def parse_sensor_data(self, raw_data: str) -> Optional[Dict[str, Any]]:
        match = re.search(r"Motion detected at (\d+)cm", raw_data)
        if match: return {"presenca": True, "distancia": int(match.group(1))}
        return None

if __name__ == "__main__":
    conf = Config()
    fiware = FiwareOrionClient(conf.BROKER_URL, conf.ENTITY_ID, conf.ENTITY_TYPE)
    sensor = SensorUart(conf.SERIAL_PORT, conf.BAUD_RATE)
    
    if fiware.check_broker_health() and (fiware.entity_exists() or fiware.create_entity()) and sensor.connect():
        logger.info("✓ Inicialização concluída. Iniciando leitura...")
        last_send = time.time()
        try:
            while True:
                raw = sensor.read_data()
                if raw:
                    parsed = sensor.parse_sensor_data(raw)
                    if parsed:
                        fiware.update_sensor_data(parsed["presenca"], parsed["distancia"])
                        last_send = time.time()
                elif time.time() - last_send > conf.SEND_INTERVAL:
                    fiware.update_sensor_data(False, 0)
                    last_send = time.time()
                time.sleep(0.1)
        except KeyboardInterrupt:
            logger.info("Encerrado.")
```

---

## 🚀 Comandos de Operação Padrão

**Executar a aplicação visualizando os logs na tela:**
```bash
python3 fiware_sensor_integration.py
```

**Consultar os dados atualizados via terminal (verificar persistência no FIWARE):**
```bash
curl -s [http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001](http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:SensorUart:001) | python3 -m json.tool
```
