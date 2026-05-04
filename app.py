import requests
import matplotlib.pyplot as plt
from IPython.display import clear_output
import time

BASE_URL = "http://34.55.148.224:8666"
ENTITY_TYPE = "lamp"
ENTITY_ID = "urn:ngsi-ld:lamp:001"

HEADERS = {
    'fiware-service': 'smart',
    'fiware-servicepath': '/'
}


# Função genérica para obter dados de qualquer atributo
def obter_dados_atributo(atributo, lastN):
    url = f"{BASE_URL}/STH/v1/contextEntities/type/{ENTITY_TYPE}/id/{ENTITY_ID}/attributes/{atributo}?lastN={lastN}"

    try:
        response = requests.get(url, headers=HEADERS, timeout=10)

        if response.status_code == 200:
            data = response.json()
            valores = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
            return valores
        else:
            print(f"Erro ao obter dados de {atributo}: {response.status_code}")
            print(response.text)
            return []

    except Exception as erro:
        print(f"Erro de conexão ao obter {atributo}: {erro}")
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

    plt.figure(figsize=(12, 5))
    plt.plot(tempos, valores, marker='o', linestyle='-', label='Valores')
    plt.axhline(media_, linestyle='--', label=f'Média: {media_:.2f}')

    plt.title(titulo)
    plt.xlabel('Tempo')
    plt.ylabel(ylabel)
    plt.xticks(rotation=45)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()


# Função que atualiza o dashboard
def atualizar_dashboard(lastN):
    clear_output(wait=True)

    print("📊 DASHBOARD DINÂMICO - FIWARE / STH-COMET")
    print("🔄 Atualizando dados...\n")

    # Obter dados
    dados_luminosidade = obter_dados_atributo("luminosity", lastN)
    dados_temperatura = obter_dados_atributo("temperature", lastN)
    dados_umidade = obter_dados_atributo("humidity", lastN)

    # Extrair séries
    tempos_luz, valores_luz = extrair_series(dados_luminosidade)
    tempos_temp, valores_temp = extrair_series(dados_temperatura)
    tempos_umid, valores_umid = extrair_series(dados_umidade)

    # Mostrar resumo
    print("📌 RESUMO DOS DADOS\n")

    if valores_luz:
        print(f"💡 Luminosidade atual: {valores_luz[-1]:.2f}")
        print(f"💡 Média da luminosidade: {sum(valores_luz) / len(valores_luz):.2f}")
    else:
        print("💡 Luminosidade: sem dados")

    if valores_temp:
        print(f"🌡️ Temperatura atual: {valores_temp[-1]:.2f} °C")
        print(f"🌡️ Média da temperatura: {sum(valores_temp) / len(valores_temp):.2f} °C")
    else:
        print("🌡️ Temperatura: sem dados")

    if valores_umid:
        print(f"💧 Umidade atual: {valores_umid[-1]:.2f} %")
        print(f"💧 Média da umidade: {sum(valores_umid) / len(valores_umid):.2f} %")
    else:
        print("💧 Umidade: sem dados")

    print("\n")

    # Plotar gráficos
    plotar_grafico(
        tempos_luz,
        valores_luz,
        "Gráfico de Luminosidade em Função do Tempo",
        "Luminosidade"
    )

    plotar_grafico(
        tempos_temp,
        valores_temp,
        "Gráfico de Temperatura em Função do Tempo",
        "Temperatura (°C)"
    )

    plotar_grafico(
        tempos_umid,
        valores_umid,
        "Gráfico de Umidade em Função do Tempo",
        "Umidade (%)"
    )

    print("✅ Dashboard atualizado com sucesso.")


# Solicitar lastN uma única vez
while True:
    try:
        lastN = int(input("Digite um valor para lastN entre 1 e 100: "))

        if 1 <= lastN <= 100:
            break
        else:
            print("O valor deve estar entre 1 e 100.")

    except ValueError:
        print("Por favor, digite um número válido.")


# Solicitar intervalo de atualização
while True:
    try:
        intervalo = int(input("Digite o intervalo de atualização em segundos: "))

        if intervalo >= 5:
            break
        else:
            print("Use um intervalo de pelo menos 5 segundos.")

    except ValueError:
        print("Por favor, digite um número válido.")


# Loop dinâmico
while True:
    atualizar_dashboard(lastN)
    time.sleep(intervalo)