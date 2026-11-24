# API do Totem (Spring Boot)

Recebe os eventos do totem ESP32, valida, guarda em H2 e mostra num painel web.

## Executar
Requisitos: Java 17 e Maven.

    export API_KEY="sua-chave"        # Windows PowerShell: $env:API_KEY="sua-chave"
    mvn spring-boot:run

Painel: http://localhost:8080  (digite a mesma chave)

A mesma chave deve estar em `API_KEY` do `secrets.h` do ESP32.

## Rotas (todas exigem o cabecalho X-API-Key)
- POST /api/leituras  -> 201 com o registro criado, 400 se o payload for invalido, 401 sem chave
- GET  /api/leituras  -> 100 leituras mais recentes
- GET  /api/resumo    -> contadores

## Teste rapido
    curl -i -X POST http://localhost:8080/api/leituras \
      -H "Content-Type: application/json" -H "X-API-Key: sua-chave" \
      -d '{"totem_id":"TOTEM_QUADRA_01","uid":"A1B2C3D4","status":"LIBERADO","offline":false,"uptime_ms":15230}'

## Usar com o Wokwi
O Wokwi nao enxerga o localhost do seu PC. Exponha a porta com um tunel:

    ngrok http 8080

e coloque a URL gerada em `API_URL` do `secrets.h`, por exemplo `http://xxxx.ngrok-free.app/api/leituras`.
