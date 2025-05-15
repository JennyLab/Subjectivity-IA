import random
import logging
from typing import Dict, List, Optional

# Configuration
MAX_RISK = 100
RISK_THRESHOLD = 98.635137
DEFAULT_PROBABILITY = 0.1
DEFAULT_CONSEQUENCE = 1
DEFAULT_UNCERTAINTY = 0

# Logging configuration
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class RiskEvaluator:
    """
    Class to evaluate event risk and decide whether they should be avoided.
    """
    def __init__(self, history: Optional[List[Dict]] = None):
        """
        Initializes the risk evaluator.
        Args:
            history: Optional history of past events.
        """
        self.history = history if history is not None else []

    def calculate_risk(self, event: Dict) -> float:
        """
        Calculates the perceived risk of an event, considering probability, consequence, and uncertainty.
        Args:
            event: A dictionary containing information about the event, including 'probability',
                   'consequence', and 'uncertainty'.
        Returns:
            The calculated risk (a value between 0 and MAX_RISK).
        """
        probability = event.get("probability", DEFAULT_PROBABILITY)
        consequence = event.get("consequence", DEFAULT_CONSEQUENCE)
        uncertainty = event.get("uncertainty", DEFAULT_UNCERTAINTY)

        risk = probability * consequence * (1 + uncertainty)
        risk = min(risk * 100, MAX_RISK)  # Ensure risk does not exceed MAX_RISK
        logging.debug(f"Calculated risk: {risk} for event: {event}")
        return risk

    def desensitization_factor(self, risk: float, history: List[Dict]) -> float:
        """
        Calculates a desensitization factor based on the history of high-risk events.
        Args:
            risk: The risk of the current event.
            history: The history of past events.
        Returns:
            The desensitization factor (a value between 0 and 1).
        """
        high_risk_events = [e for e in history if e.get("risk", 0) >= 80]
        count = len(high_risk_events)
        if risk < 30:
            factor = min(0.8, 0.65 + count * 0.01)
            logging.debug(f"Low risk ({risk}), desensitization factor: {factor}")
        elif risk >= 98:
            factor = 0.0
            logging.debug(f"Critical risk ({risk}), no desensitization")
        else:
            factor = 1.0 - min(0.35, count * 0.01)
            logging.debug(f"Medium risk ({risk}), desensitization factor: {factor}")
        return factor

    def necessity_weight(self, necessity: float) -> float:
        """
        Calculates a weight for the necessity of an event, using a sigmoid function.
        Args:
            necessity: The necessity of the event (a value between 0 and 100).
        Returns:
            The necessity weight (a value between 0 and 1).
        """
        if necessity == 0:
            return 0
        # We use a sigmoid function to model necessity non-linearly
        k = 0.1  # Adjust the slope of the sigmoid curve
        weighted_necessity = 1 / (1 + math.exp(-k * (necessity - 50)))
        logging.debug(f"Necessity: {necessity}, weighted necessity: {weighted_necessity}")
        return weighted_necessity

    def should_avoid(self, event: Dict, history: List[Dict]) -> bool:
        """
        Decides whether an event should be avoided, considering risk and necessity.
        Args:
            event: A dictionary containing information about the event, including 'risk' and 'necessity'.
            history: The history of past events.
        Returns:
            True if the event should be avoided, False otherwise.
        """
        risk = self.calculate_risk(event)
        factor = self.desensitization_factor(risk, history)
        need = self.necessity_weight(event.get("necessity", 0))
        decision_weight = (risk / MAX_RISK) * factor
        combined = (decision_weight + need) / 2

        logging.info(f"Event: {event}, Risk: {risk}, Desensitization factor: {factor}, Need: {need}, Combined weight: {combined}")
        return combined > 0.5

def simulate_decision_making(events: List[Dict], num_iterations: int = 10) -> List[Dict]:
    """
    Simulates decision-making about a list of events over several iterations,
    showing the effect of desensitization to risk.
    Args:
        events: A list of dictionaries, where each dictionary represents an event
                and contains information about risk and necessity.
        num_iterations: The number of iterations to simulate.
    Returns:
        A list of dictionaries, where each dictionary contains the length of the history
        and the decision results for each event in that iteration.
    """
    history = []
    results = []
    risk_evaluator = RiskEvaluator() # Instance of the RiskEvaluator class
    for i in range(num_iterations):
        history.append({"risk": random.randint(80, 100)})  # Add a random high-risk event
        iteration_results = {"history_len": len(history), "results": []}
        for event in events:
            decision = risk_evaluator.should_avoid(event, history)
            iteration_results["results"].append({"event": event, "decision": decision})
        results.append(iteration_results)
        logging.info(f"Iteration {i+1}: History length = {len(history)}")
    return results

if __name__ == "__main__":
    # Test events
    test_events = [
        {"risk": 10, "necessity": 0},
        {"risk": 10, "necessity": 95},
        {"risk": 85, "necessity": 0},
        {"risk": 99, "necessity": 95},
        {"risk": 25, "necessity": 85},
    ]

    # Simulation
    simulation_results = simulate_decision_making(test_events)

    # Print simulation results
    for iteration_result in simulation_results:
        print(f"History Length: {iteration_result['history_len']}")
        for result in iteration_result['results']:
            event = result['event']
            decision = result['decision']
            print(f"  Event: {event}, Should Avoid: {decision}")
