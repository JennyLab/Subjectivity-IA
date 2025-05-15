import random

# Variables de configuracion
MAX_RISK = 100  # Riesgo maximo
RISK_THRESHOLD = 98.635137  # Umbral superior para necesidad
HISTORY = []  # Historial de eventos

# Función de riesgo percibido (simulacion basica)
def calculate_risk(event):
    return event.get("risk", 0)

# Funcion de desensibilizacion (mayor historial, menor reaccion a bajo riesgo)
def desensitization_factor(risk, history):
    high_risk_events = [e for e in history if e["risk"] >= 80]
    count = len(high_risk_events)
    if risk < 30:
        factor = min(0.8, 0.65 + count * 0.01)  # Aumenta desensibilizacion
    elif risk >= 98:
        factor = 0.0  # No se ignoran riesgos criticos
    else:
        factor = 1.0 - min(0.35, count * 0.01)
    return factor

# Evaluacion de la necesidad
def necessity_weight(necessity):
    if necessity == 0:
        return 0
    pct = min(max(necessity, 80.0), RISK_THRESHOLD) / 100.0
    return pct

# Decision final
def should_avoid(event, history):
    risk = calculate_risk(event)
    factor = desensitization_factor(risk, history)
    need = necessity_weight(event.get("necessity", 0))
    decision_weight = (risk / MAX_RISK) * factor
    combined = (decision_weight + need) / 2
    return combined > 0.5

# Eventos de prueba
test_events = [
    {"risk": 10, "necessity": 0},       # Bajo riesgo, sin necesidad
    {"risk": 10, "necessity": 95},      # Bajo riesgo, alta necesidad
    {"risk": 85, "necessity": 0},       # Alto riesgo, sin necesidad
    {"risk": 99, "necessity": 95},      # Riesgo crítico, con necesidad
    {"risk": 25, "necessity": 85},      # Riesgo medio-bajo, necesidad moderada
]

# Simulacion con historial creciente
results = []
for i in range(10): 
    HISTORY.append({"risk": random.randint(80, 100)})
    iteration = {"history_len": len(HISTORY), "results": []}
    for event in test_events:
        decision = should_avoid(event, HISTORY)
        iteration["results"].append((event, decision))
    results.append(iteration)

results
