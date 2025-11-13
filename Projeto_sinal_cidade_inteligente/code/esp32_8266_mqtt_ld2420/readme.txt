no node red a configuracao e:

mqqt in ----> debug
mqqt in config: login e senha se quiser, usar ip da maquina que esta rodando o mqtt ou 127.0.0.1 se for na mesma que esta rodadno o codigo. 
colocar o "caminho" do topico como esta no codigo.

debug config : padrao


erros :
  "#include <WiFi.h>" nao funciona no esp8266. tem que usar #include <ESP8266WiFi.h>
