# Totem de Acesso NFC/RFID (ESP32 + API Spring Boot)

Trabalho final de Internet das Coisas, 2º semestre de 2026.

O totem lê a tag NFC/RFID, **decide localmente** se o acesso é liberado (primeira leitura) ou negado (tag já usada), sinaliza com LEDs e buzzer e envia o evento em JSON, por Wi-Fi (HTTPS), a uma **API Spring Boot** que guarda o histórico e o exibe num painel web. Sem internet, o totem continua decidindo sozinho e reenvia os eventos depois.

**Projeto no Wokwi:** https://wokwi.com/projects/476530242224523265

## Arquitetura (quatro camadas)

| Camada | Componentes |
|---|---|
| Percepção | Leitor MFRC522, tags NFC/RFID; atuadores: LED verde, LED vermelho e buzzer |
| Rede | Wi-Fi do ESP32, HTTPS POST com cabeçalho `X-API-Key`, em tarefa FreeRTOS própria (Core 0) |
| Processamento | Firmware no ESP32: conjunto de UIDs lidos (RAM + flash) e fila de eventos com confirmação de envio |
| Aplicação | API Spring Boot (validação, banco H2) e painel web |

## Estrutura do repositório

```
.
├── README.md
├── .gitignore
├── firmware/                 (cópia dos arquivos do projeto no Wokwi)
│   ├── sketch.ino
│   ├── diagram.json          (circuito)
│   ├── libraries.txt         (MFRC522)
│   └── secrets.example.h     (modelo das credenciais)
└── api-totem/
    ├── pom.xml
    ├── README.md
    └── src/main/...
```

O `.gitignore` da raiz deve conter:

```
firmware/secrets.h
api-totem/target/
api-totem/data/
```

## Requisitos

- Navegador e conta no [Wokwi](https://wokwi.com) (o firmware roda no simulador, sem Arduino IDE)
- Biblioteca **MFRC522**, adicionada ao projeto do Wokwi
- Java 17 e Maven, para a API
- Um túnel (por exemplo `pinggy`), porque o Wokwi não alcança o `localhost` do seu computador

## Como executar em outra máquina

### 1. Subir a API

**Windows (PowerShell):**

```powershell
cd api-totem
$env:API_KEY="sua-chave"
mvn spring-boot:run
```

**Linux/macOS:**

```bash
cd api-totem
export API_KEY="sua-chave"
mvn spring-boot:run
```

A variável vale só para a janela do terminal em que foi definida. Se preferir rodar pelo IntelliJ, abra `TotemApiApplication.java`, vá em **Run > Edit Configurations > Environment variables**, informe `API_KEY=sua-chave` e clique no triângulo verde do `main` (nesse caso não é preciso ter o Maven instalado no terminal).

A API sobe em `http://localhost:8080`. Abra esse endereço no navegador, digite a chave e o painel mostra as leituras (atualiza a cada 2 s).

### 2. Expor a API ao Wokwi

O Wokwi roda na internet e não enxerga o `localhost` do seu computador. Com a API ligada, abra um túnel em outro terminal:

```bash
ssh -p 443 -R0:localhost:8080 a.pinggy.io
```

Copie a URL **https** gerada (algo como `https://xxx.run.pinggy-free.link`). O firmware usa HTTPS, então a URL precisa começar com `https://`. Mantenha a API e o túnel ligados. No plano gratuito a URL muda a cada vez que o túnel é reiniciado: atualize o `secrets.h` sempre que isso acontecer.

### 3. Montar o projeto no Wokwi

1. Em wokwi.com, crie um projeto **ESP32** (ou abra o link do projeto).
2. Substitua o conteúdo de `sketch.ino` pelo de `firmware/sketch.ino`.
3. Na aba `diagram.json`, cole o conteúdo de `firmware/diagram.json` (circuito com as ligações da tabela abaixo).
4. Adicione a biblioteca **MFRC522** pelo gerenciador de bibliotecas do Wokwi (ou confira se está em `libraries.txt`).
5. Crie um arquivo novo chamado `secrets.h` no projeto, com o conteúdo de `firmware/secrets.example.h`, e preencha:

| Campo | Valor |
|---|---|
| `WIFI_SSID` / `WIFI_PASS` | `Wokwi-GUEST` e senha vazia |
| `API_URL` | `https://xxx.run.pinggy-free.link` (a URL https do túnel, com `/api/leituras`) |
| `API_KEY` | A mesma chave usada em `API_KEY` na API |

No repositório só existe o `secrets.example.h`; o `secrets.h` real não vai para o Git.

### 4. Executar

Clique em **Start the simulation**. O LED de status (D2) acende fixo quando o Wi-Fi conecta e não há eventos pendentes. Aproxime uma tag: clicando no sensor MFRC522 e depois em tap. A leitura aparece no painel da API (`http://localhost:8080`) em até 2 segundos.

## Ligações

| Componente | Pino do componente | Pino do ESP32 |
|---|---|---|
| MFRC522 | SDA (SS) | D5 |
| MFRC522 | SCK | D18 |
| MFRC522 | MOSI | D23 |
| MFRC522 | MISO | D19 |
| MFRC522 | RST | D22 |
| MFRC522 | 3.3V / GND | 3V3 / GND |
| LED verde (com resistor) | Ânodo | D26 |
| LED vermelho (com resistor) | Ânodo | D25 |
| Buzzer | Positivo | D27 |
| LED de status (da placa) | – | D2 |

LED de status (D2):

| Estado do LED | Significado |
|---|---|
| Aceso fixo | Wi-Fi conectado e nenhum evento pendente |
| Piscando lento (500 ms) | Wi-Fi conectado, enviando eventos pendentes |
| Piscando rápido (150 ms) | Sem Wi-Fi |

## Como o sistema se comporta

1. A tag é aproximada e o UID é lido (um intervalo mínimo de 250 ms evita leitura dupla no mesmo toque).
2. Se o UID **não** está no conjunto de tags lidas: acesso **liberado** (LED verde, dois bipes), UID gravado na RAM e na flash.
3. Se o UID **já** está no conjunto: acesso **negado** (LED vermelho, bipe grave).
4. O evento entra numa fila de até 50 itens. Uma **tarefa de rede separada** (Core 0) envia o evento por HTTPS e só o remove da fila depois de uma resposta 2xx da API. Se o envio falhar, ela espera 2 s e tenta de novo.

A decisão dos passos 2 e 3 e os sinais de LED e buzzer (feitos sem `delay()`) não dependem de rede nem da API.

### Comandos no console serial

| Comando | O que faz |
|---|---|
| `STATUS` | Mostra Wi-Fi, quantidade de tags, tamanho da fila e memória livre |
| `RESET` | Apaga as tags da RAM e da flash (útil para repetir os testes) |

## Payload (JSON)

| Campo | Tipo | Descrição |
|---|---|---|
| `totem_id` | texto | Identificador do totem (até 50 caracteres) |
| `uid` | texto | UID da tag, hexadecimal maiúsculo (4 a 20 caracteres) |
| `status` | texto | `LIBERADO` ou `NEGADO_DUPLICADO` |
| `offline` | booleano | `true` se o evento foi gerado sem Wi-Fi ou só foi enviado depois de uma falha de envio |
| `uptime_ms` | inteiro | Milissegundos desde que o ESP32 ligou |

Exemplo:

```json
{
  "totem_id":"TOTEM_QUADRA_01",
  "uid":"A1B2C3D4",
  "status":"LIBERADO",
  "offline":false,
  "uptime_ms":15230
}
```

## API

Todas as rotas `/api/**` exigem o cabeçalho `X-API-Key`.

| Método | Rota | Resposta |
|---|---|---|
| POST | `/api/leituras` | `201` com o registro criado; `400` payload inválido; `401` chave inválida |
| GET | `/api/leituras` | As 100 leituras mais recentes |
| GET | `/api/resumo` | Contadores: `total`, `liberados`, `negados`, `offline` |

O histórico fica num banco H2 em arquivo (`api-totem/data/`), então sobrevive a reinicializações da API.

## Segurança

- Credenciais (Wi-Fi, URL, chave) fora do repositório: `secrets.h` está no `.gitignore`.
- Chave de API no cabeçalho `X-API-Key`, comparada em tempo constante no servidor; na API ela vem da variável de ambiente `API_KEY`.
- Comunicação por HTTPS (`WiFiClientSecure`).
- Toda entrada da API é validada (formato do UID, valores de status, tamanho dos campos).
- O painel escapa o texto recebido antes de exibi-lo.

**Limitações conhecidas:**

- O firmware usa `client.setInsecure()`: a conexão é criptografada, mas o certificado do servidor **não é validado**, então um invasor na rede poderia se passar por ele. Melhoria futura: `client.setCACert()` com o certificado raiz.
- Os comandos `RESET` e `STATUS` pelo console serial não pedem autenticação (quem tem acesso físico ao totem pode apagar as tags).
- Troque a chave de exemplo (`troque-esta-chave`) antes de qualquer uso real.
- No Wokwi, o `secrets.h` fica visível para quem abrir o link compartilhado. Use uma chave criada só para a demonstração, que não seja usada em mais nada.
- Os UIDs ficam na flash sem criptografia.

## Limitações do protótipo

- A fila de eventos fica na RAM e se perde se o ESP32 reiniciar.
- Com a fila cheia, o evento mais antigo é descartado; no caso raro de ele estar sendo enviado nesse instante, pode ser removido sem confirmação.
- A lista de UIDs numa única string da flash comporta cerca de 4000 bytes; ao chegar nesse limite a tag fica só na RAM e o sistema avisa no Serial.
- Não há reset de tags pela rede (só o comando `RESET` no console) nem atualização OTA.

## Integrantes

Matheus Henrique Barleta Dias

## Uso de IA

A IA (Claude) foi usada como apoio em:

- escrita da API Spring Boot;
- melhor compreensão das bibliotecas usadas no Wokwi;
- criação dos arquivos README.

O resultado foi verificado pelo próprio aluno, com:

- testes do sistema em funcionamento;
- compilação do código no Wokwi;
- subida da API com o túnel Pinggy.