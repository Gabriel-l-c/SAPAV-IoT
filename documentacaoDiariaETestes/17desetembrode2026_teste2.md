```markdown
# RELATÓRIO TÉCNICO EXPERIMENTAL: CAPTURA DE SÉRIE TEMPORAL E VALIDAÇÃO DE FILTRO DE BORDA

**Instituição:** Universidade Federal do Espírito Santo (UFES) — Centro Tecnológico  
**Laboratório:** Equipe de Robótica da UFES (ERUS)  
**Autor:** Gabriel Lyra Campos  
**Data:** 17 de setembro de 2026  

---

## 1. Introdução e Hipótese
O Roteiro 2 teve como objetivo validar a capacidade da arquitetura distribuída Edge-Gateway em rastrear dinâmicas espaciais contínuas (séries temporais) de movimentação humana. Adicionalmente, buscou-se avaliar a eficácia do processamento na borda (*Edge Computing*) na mitigação do ruído de hardware. A hipótese central assume que a delegação de um filtro de variação espacial (*deadband*) diretamente ao microcontrolador reduz o *overhead* de rede e evita o registro de falsos positivos (efeito fantasma) no banco de dados FIWARE Orion-LD, contornando a ausência de reconhecimento semântico nativo do sensor de radar mmWave.

## 2. Metodologia Experimental
O *setup* físico foi posicionado ao longo de um corredor isolado para maximizar o campo de detecção linear e longitudinal do sistema. A topologia foi executada em três frentes de processamento:

* **Nó Sensor (Edge - Aquisição e Filtragem):** O radar LD2420 comunicou-se com o microcontrolador ESP8266 via `SoftwareSerial` sob uma taxa corrigida de 115200 baud. O firmware foi programado para aplicar um limiar de transição de 5 cm, emitindo pacotes via porta USB estritamente quando o deslocamento absoluto do alvo ultrapassasse essa margem de tolerância.
* **Gateway IoT (Encapsulamento e Transporte):** Um Raspberry Pi 4 capturou a telemetria serial brutos via *script* Python. O *script* formatou dinamicamente os pacotes em *payloads* aderentes ao padrão ETSI NGSI-LD (`application/ld+json`, tenant: `openiot`) e realizou requisições HTTP `POST` (*upsert*) para o Context Broker local.
* **Coleta em Paralelo (Polling):** Um terminal simultâneo no Raspberry Pi rodou um *script* de captura (`serie_temporal_poll.py`), consultando a API REST do Orion-LD a um intervalo configurado de 1,5 segundos. Os atributos de presença e distância foram estruturados em um log tabular (CSV) e renderizados graficamente em tempo real.

## 3. Resultados e Análise Quantitativa
O ensaio físico registrou a movimentação ao longo de **206,2 segundos** (~3,4 minutos) de captura contínua. O mecanismo de *polling* extraiu um consolidado de **136 amostras válidas**, operando com um intervalo médio real de comunicação de **1,53 segundos** com o servidor FIWARE. O domínio espacial validado cobriu aproximações quase completas, variando de um mínimo de **9 cm** a um máximo de **600 cm** de distância linear.

A representação gráfica (`serie_temporal_2.png`) demonstra quatro ciclos primários de percurso no corredor:

* **Rampas de Aproximação:** Descidas contínuas e acentuadas no gráfico de distância, marcando a progressão do alvo em direção ao sistema até curtas distâncias (ex: queda até 9 cm ao redor do instante de 160s).
* **Rampas de Afastamento:** Subidas graduais, marcando o recuo do alvo de volta ao limite de captação do radar sem perdas de pacotes durante a mudança de sentido.
* **Detecção de Presença (Booleana):** O registro booleano manteve-se constantemente em `1` (*True*), provando que não houve "pontos cegos" de rastreamento do alvo durante todo o intervalo em que esteve na área útil do corredor.

## 4. Discussão (Validação do Processamento na Borda)
A análise dos dados temporais comprova empiricamente a eficácia da lógica de filtragem de *hardware* delegada ao ESP8266. No gráfico de distância, observam-se "platôs" estacionários perfeitamente retilíneos nos instantes em que o alvo interrompeu a caminhada.

Destacam-se duas janelas de estabilidade absoluta nos dados processados:

1. **Platô Intermediário:** Entre os instantes **108,4s e 132,8s** (duração de ~24,4 segundos), o alvo permaneceu estático. A distância foi matematicamente travada em **536 cm** durante 17 amostras consecutivas.
2. **Platô de Limite:** Entre os instantes **183,4s e 198,6s** (duração de ~15,2 segundos), a medição estabilizou de forma linear no teto de **589 cm**.

Durante esses períodos de imobilidade, o ruído térmico natural das antenas de micro-ondas e os micromovimentos fisiológicos (como a respiração) foram suprimidos localmente pelo limite de 5 cm. Como resultado direto, o ESP8266 silenciou a comunicação serial, eximindo o Raspberry Pi de processar *payloads* redundantes e impedindo requisições HTTP desnecessárias. O banco de dados FIWARE manteve o último estado íntegro e congelado, demonstrando excelente otimização de banda de rede e processamento em nuvem.

## 5. Conclusão
A arquitetura se provou robusta e altamente eficaz para o monitoramento contínuo em ambientes reais. O particionamento inteligente de recursos computacionais — onde o microcontrolador assume a filtragem primária local de *hardware* e o *Single-Board Computer* responsabiliza-se unicamente pelo encapsulamento semântico NGSI-LD — assegurou uma ingestão de dados limpa. Esta topologia validou a hipótese de que sensores passíveis de falso-positivo, quando combinados a nós de *Edge Computing*, são capazes de alimentar plataformas de *Smart Cities* (como o FIWARE) com alta precisão e baixo custo computacional.

```
