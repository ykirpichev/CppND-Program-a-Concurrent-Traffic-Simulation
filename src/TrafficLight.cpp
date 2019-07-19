#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock lk(_mutex);
    _condition.wait(lk, [this](){ return !_queue.empty();});
    auto entry = std::move(_queue.front());
    _queue.pop_front();
    return entry;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard lk(_mutex);
    _queue.push_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        auto phase = _messages.receive();
        if (phase == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard lk(_mutex);
    return _currentPhase;
}

void TrafficLight::setCurrentPhase(TrafficLightPhase newPhase)
{
    std::lock_guard lk(_mutex);
    _currentPhase = newPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back([this](){cycleThroughPhases();});
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device rd;     // only used once to initialise (seed) engine
    std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int> uni(4000, 6000); // guaranteed unbiased

    auto lastToggleTime = std::chrono::system_clock::now();
    auto toggleDuration = std::chrono::milliseconds(uni(rng));
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto currentTime = std::chrono::system_clock::now();
        if (currentTime - lastToggleTime >= toggleDuration) {
            lastToggleTime = currentTime;
            toggleDuration = std::chrono::milliseconds(uni(rng));
            if (getCurrentPhase() == TrafficLightPhase::green) {
                setCurrentPhase(TrafficLightPhase::red);
                _messages.send(std::move(TrafficLightPhase::red));
            } else {
                 setCurrentPhase(TrafficLightPhase::green);
                _messages.send(std::move(TrafficLightPhase::green));
            }
        }
    }
}
