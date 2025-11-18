# antes deve instalar no so "pip install paho-mqtt"

import paho.mqtt.client as mqtt

# =================================================================
# CONFIGURAÇÕES MQTT
# =================================================================

MQTT_SERVER = "172.16.30.83" # O mesmo IP do seu código Arduino
MQTT_PORT = 1883
MQTT_TOPIC_DISTANCE = "radar/presenca/distancia"
MQTT_TOPIC_STATE = "radar/presenca/estado"
CLIENT_ID = "RaspberryPi_Subscriber"

# =================================================================
# FUNÇÕES DE CALLBACK
# =================================================================

# Chamado quando o cliente recebe uma resposta CONNACK do broker
def on_connect(client, userdata, flags, rc):
    print(f"Conectado ao broker MQTT com código de resultado {rc}")
    # Se reconectar, resubscreve
    client.subscribe(MQTT_TOPIC_DISTANCE)
    client.subscribe(MQTT_TOPIC_STATE)
    print(f"Subscrito aos tópicos: {MQTT_TOPIC_DISTANCE} e {MQTT_TOPIC_STATE}")

# Chamado quando uma mensagem for recebida de um tópico subscrito
def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode()
        
        if topic == MQTT_TOPIC_DISTANCE:
            # O payload é a distância em centímetros (ex: "125")
            print(f"▶️ Distância Recebida: {payload} cm")
        
        elif topic == MQTT_TOPIC_STATE:
            # O payload é o estado (ex: "Movimento" ou "Presenca_Parada")
            print(f"⚡ Estado Recebido: {payload}")
        
    except Exception as e:
        print(f"Erro ao processar mensagem: {e}")

# =================================================================
# CÓDIGO PRINCIPAL
# =================================================================

def main():
    print("Iniciando cliente Raspberry Pi MQTT...")
    client = mqtt.Client(CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
    except Exception as e:
        print(f"✗ Falha ao conectar ao MQTT Broker: {e}")
        return

    # Loop para manter o cliente rodando e escutando mensagens
    client.loop_forever()

if __name__ == "__main__":
    main()
