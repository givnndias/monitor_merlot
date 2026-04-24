# ---------------------------------------------------------------------------
# Projeto: Dashboard da Vinheria Inteligente
# Autor: Giovanna Oliveira Ferreira Dias
# Descrição:
#   Este dashboard Flask consome dados históricos do STH-Comet (FIWARE)
#   e exibe gráficos dinâmicos de temperatura, umidade e luminosidade
#   em tempo real utilizando a biblioteca Plotly.
#   O dashboard é hospedado na VM Linux (Ubuntu 24.04) e roda como um
#   serviço systemd, acessível via porta 5000.
# ---------------------------------------------------------------------------


import requests
import matplotlib.pyplot as plt

BASE_URL = "http://34.55.209.28:8666"
ENTITY_TYPE = "lamp"
ENTITY_ID = "urn:ngsi-ld:lamp:001"

HEADERS = {
    'fiware-service': 'smart',
    'fiware-servicepath': '/'
}


# Função genérica para obter dados de qualquer atributo
def obter_dados_atributo(atributo, lastN):
    url = f"{BASE_URL}/STH/v1/contextEntities/type/{ENTITY_TYPE}/id/{ENTITY_ID}/attributes/{atributo}?lastN={lastN}"

    response = requests.get(url, headers=HEADERS)

    if response.status_code == 200:
        data = response.json()
        valores = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
        return valores
    else:
        print(f"Erro ao obter dados de {atributo}: {response.status_code}")
        return []


# Função para extrair tempos e valores
def extrair_series(dados):
    if not dados:
        return [], []

    valores = []
    tempos = []

    for entry in dados:
        try:
            valores.append(float(entry['attrValue']))
            tempos.append(entry['recvTime'])
        except (ValueError, KeyError, TypeError):
            continue

    return tempos, valores


# Função para plotar um gráfico
def plotar_grafico(tempos, valores, titulo, ylabel):
    if not valores:
        print(f"Nenhum dado disponível para plotar {titulo}.")
        return

    media_ = sum(valores) / len(valores)

    plt.figure(figsize=(12, 6))
    plt.plot(tempos, valores, marker='o', linestyle='-')
    plt.axhline(media_, linestyle='--', label=f'Média: {media_:.2f}')

    plt.title(titulo)
    plt.xlabel('Tempo')
    plt.ylabel(ylabel)
    plt.xticks(rotation=45)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()


# Solicitar ao usuário um valor lastN entre 1 e 100
while True:
    try:
        lastN = int(input("Digite um valor para lastN (entre 1 e 100): "))
        if 1 <= lastN <= 100:
            break
        else:
            print("O valor deve estar entre 1 e 100. Tente novamente.")
    except ValueError:
        print("Por favor, digite um número válido.")


# Obter dados
dados_luminosidade = obter_dados_atributo("luminosity", lastN)
dados_temperatura = obter_dados_atributo("temperature", lastN)
dados_umidade = obter_dados_atributo("humidity", lastN)

# Extrair séries
tempos_luz, valores_luz = extrair_series(dados_luminosidade)
tempos_temp, valores_temp = extrair_series(dados_temperatura)
tempos_umid, valores_umid = extrair_series(dados_umidade)

# Plotar gráficos
plotar_grafico(tempos_luz, valores_luz, "Gráfico de Luminosidade em Função do Tempo", "Luminosidade")
plotar_grafico(tempos_temp, valores_temp, "Gráfico de Temperatura em Função do Tempo", "Temperatura (°C)")
plotar_grafico(tempos_umid, valores_umid, "Gráfico de Umidade em Função do Tempo", "Umidade (%)")