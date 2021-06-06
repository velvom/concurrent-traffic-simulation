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
    
    // Wait for and receive new messages
    std::unique_lock<std::mutex> lck(_mutex);
    _cond.wait(lck, [this] { return !_queue.empty(); });

    // Remove from the back
    T msg = std::move(_queue.front());
    _queue.pop_front();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform deque modification under the lock
    std::lock_guard<std::mutex> lck(_mutex);

    // add message to deque
    //std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _queue.clear();
    _queue.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new message into deque
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _tlphasequeue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        auto message = _tlphasequeue->receive();
        if (message == TrafficLightPhase::green) {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    std::random_device rd;
    static std::mt19937 e{rd()}; // static since creating a new engine is time consuming
    static std::uniform_int_distribution<long> udist{4000, 6000};

    long cycleDuration = udist(e); // For the random number generated need to be outside the loop
    long timeSinceLastUpdate;

    // init stop watch
    std::chrono::time_point<std::chrono::system_clock> lastUpdate = std::chrono::system_clock::now();

    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            // Toggle between Red and Green
            std::unique_lock<std::mutex> lck(_mutex);
            _currentPhase = (_currentPhase == TrafficLightPhase::green) ? TrafficLightPhase::red : TrafficLightPhase::green;
            lck.unlock();

            // Send update message to TrafficLightPhase queue: _tlphasequeue"
            _tlphasequeue->send(std::move(_currentPhase));
            //auto ftrUpdateMsg = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _tlphasequeue, std::move(_currentPhase));

            // wait until TrafficLightPhase queue has been updated
            //ftrUpdateMsg.get();

            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            //Picking another random number
            cycleDuration = udist(e);
        }
    }
}
