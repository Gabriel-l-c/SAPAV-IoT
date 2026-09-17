/*
 * latencia_firmware_teste.ino
 * -----------------------------
 * Firmware de TESTE (nao e o firmware de producao) para o roteiro de
 * "latencia de transporte ponta a ponta". Em vez de depender do radar
 * LD2420 e de um acionamento manual (mao na frente do sensor + Enter,
 * que tem erro de reacao humana de ~150-300 ms), este sketch ignora o
 * radar e envia, sozinho, um pacote JSON pela serial USB a um intervalo
 * FIXO e conhecido (1000 ms), numerado sequencialmente.
 *
 * Como o instante de envio de cada pacote e determinado pelo proprio
 * clock do microcontrolador (millis()), nao ha "reflexo humano" a
 * medir: qualquer atraso ou perda observado do lado do Raspberry Pi e,
 * por definicao, atraso/perda do transporte (serial + USB + gateway +
 * rede + broker), que e exatamente o que queremos isolar.
 *
 * A distancia enviada varia a cada pacote (regra abaixo) para que,
 * caso o script do lado do RPi ainda tenha algum filtro de zona morta
 * ativo, o pacote nao seja descartado por parecer "sem mudanca".
 *
 * Depois do teste, grave novamente o firmware de producao normal
 * (o que le o LD2420 de verdade).
 *
 * Uso:
 *   1. Grave este sketch no ESP8266.
 *   2. Abra o Monitor Serial (ou va direto pro latencia_gateway_periodico.py
 *      no Raspberry Pi) e confira que os pacotes chegam a cada ~1000 ms.
 */

unsigned long seq = 0;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_MS = 1000;
const unsigned long ATRASO_INICIAL_MS = 3000; // estabiliza a porta serial/USB antes de comecar

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("{\"status\": \"Firmware de teste de latencia iniciado\"}");
  delay(ATRASO_INICIAL_MS);
  ultimoEnvio = millis();
}

void loop() {
  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_MS) {
    ultimoEnvio = agora;
    seq++;

    // distancia sintetica, sempre variando >= 5 cm em relacao ao pacote anterior
    int distanciaSimulada = 50 + (int)((seq % 20)) * 5;

    Serial.print("{\"seq\": ");
    Serial.print(seq);
    Serial.print(", \"t_esp_ms\": ");
    Serial.print(agora);
    Serial.print(", \"presence\": true, \"distance\": ");
    Serial.print(distanciaSimulada);
    Serial.println("}");
  }
}
