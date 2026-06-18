# ColdGuard 🛡️❄️ — Sistema Resiliente de Telemetria e Segurança para a Cadeia de Frio

O **ColdGuard** é uma solução computacional ponta a ponta (IoT à Nuvem) projetada para o monitoramento crítico de temperatura e umidade em contêineres frigoríficos durante o transporte de cargas termo-sensíveis de alto valor, como vacinas de RNAm (que exigem temperaturas de até -80°C) e alimentos nobres.

O foco central deste projeto é a **resiliência e a segurança digital**, garantindo que nenhum dado de telemetria seja perdido devido a apagões de rede (zonas de sombra nas estradas) ou tentativas de sabotagem física e digital (como ataques de *Jamming*).

---

## 1. Arquitetura de Redes e Fluxo da Informação

A informação trafega de forma hierárquica por três camadas bem definidas, otimizando o consumo de banda e blindando o perímetro de comunicação:

1. **A Borda (Edge):** Nós sensores baseados no microcontrolador **ESP32** coletam continuamente dados de temperatura e umidade dentro do baú frigorífico.
2. **O Gateway de Bordo:** Como o baú metálico do caminhão bloqueia sinais externos (Gaiola de Faraday), os nós enviam os dados localmente via **Bluetooth Low Energy (BLE)** para um Gateway centralizador instalado na cabine do motorista.
3. **A Nuvem (Cloud):** O Gateway consolida as informações e as transmite para o broker **AWS IoT Core** usando a rede celular **4G/5G**. Caso o caminhão entre em uma zona de sombra sem sinal de celular nas rodovias, o sistema realiza um *failover* (transição automática) para a rede de **Satélite Iridium**.

### Protocolos Utilizados
* **MQTT sobre TLS (MQTTS - Porta 8883):** Escolhido em detrimento do HTTP por operar no modelo *Publish/Subscribe* com um cabeçalho extremamente leve (2 bytes). Isso economiza dados na rede móvel e satelital, além de oferecer suporte nativo a conexões instáveis via *Quality of Service (QoS 1)*.
* **HTTPS (Porta 443):** Utilizado exclusivamente para a visualização dos dados da frota no painel da central de logística, aplicando criptografia assimétrica para total sigilo das rotas.

---

## 2. Mapeamento e Matriz de Risco

A severidade de cada risco técnico em potencial foi mapeada cruzando a **Facilidade de Ocorrência** com o **Dano ao Ativo** ($\text{Risco} = \text{Facilidade} \times \text{Dano}$):

| Categoria | Falha Técnica Potencial | Facilidade | Dano | Justificativa Analítica |
| :--- | :--- | :--- | :--- | :--- |
| **01. Hardware** | Congelamento e degradação química da bateria do sensor devido ao frio extremo (-80°C). | **Alta** | **Alto** | Baterias comuns perdem carga no frio. Se o sensor apagar, o lote de vacinas perde o histórico térmico e é descartado por normas da ANVISA. |
| **02. Software** | Estouro de pilha (*Buffer Overflow*) na memória do Gateway ao acumular dados localmente em longas rotas offline. | **Média** | **Alto** | Tentar registrar infinitas leituras sem uma estrutura de dados limitada causa o travamento (*crash*) do sistema operacional embarcado, corrompendo o banco de dados local. |
| **03. Redes / Seg.** | Bloqueio total de radiofrequência celular e GPS via dispositivo rastreador ilegal (*Ataque de Jamming*). | **Média** | **Crítico** | Criminosos utilizam *jammers* para isolar o caminhão da central, permitindo o roubo da carga sem o acionamento de alarmes de desvio de rota. |

---

## 3. Plano de Mitigação Implementado no Código

O código-fonte presente na pasta `/firmware` foi desenvolvido aplicando técnicas de **programação defensiva** e arquitetura resiliente para neutralizar as ameaças listadas:

* **Defesa de Hardware (Bateria & Redundância):** O projeto prevê a substituição de células comuns por baterias industriais de **Lítio-Cloreto de Tionila ($Li-SOCl_2$)**, operacionais de -55°C a +85°C.
* **Defesa de Software (Trava Lógica FIFO e Sanitização):** O firmware do ESP32 adota uma fila circular **FIFO (First In, First Out)** com tamanho máximo restrito estaticamente (`MAX_BUFFER_RECORDS`). Se o buffer encher durante uma viagem offline, o sistema descarta o registro histórico mais antigo de forma controlada para abrir espaço para o dado atual, evitando o estouro de memória. Além disso, condicionais `if/else` barram dados anômalos (lixo ou ruído de leitura) antes de entrarem na fila.
* **Hardware Watchdog Timer (WDT):** O cão de guarda por hardware é alimentado a cada ciclo limpo do código (`esp_task_wdt_reset()`). Caso o firmware congele por mais de 10 segundos, o processador sofre um reinício limpo forçado automaticamente.
* **Defesa de Rede (MQTTS & Heartbeat contra Jamming):** O tráfego industrial usa autenticação mútua via certificados digitais X.509 (**mTLS**). Contra o *Jamming*, a nuvem espera um pulso de vida (*Heartbeat*) do Gateway a cada 60 segundos; se o caminhão sumir abruptamente da rede por 3 minutos consecutivos com a viagem ativa, a nuvem interpreta o silêncio de rede como um sinistro em andamento e dispara um alerta de pânico automatizado com a última coordenada GPS válida.

---

## 4. Estrutura do Repositório

* `/firmware/main.cpp`: Código-fonte em C++ para o ecossistema ESP32 (lógica de proteção de software, WDT e fila FIFO).
* `/config/mosquitto.conf`: Arquivo de configuração de segurança estrita para o broker MQTT utilizando criptografia na porta 8883.

---
*Desenvolvido como projeto para a Avaliação Final de Introdução à Computação - Junho de 2026.*
