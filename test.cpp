#include "fixed-thread-pool.h"
#include "multi-queue-thread-pool.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    FixedThreadPool pool{8};
    MultiQueueThreadPool mqpool{pool};

    QueueHandle john = mqpool.create_queue();
    QueueHandle paul = mqpool.create_queue();
    QueueHandle george = mqpool.create_queue();

    const int jobs_per = 100;

    std::vector<int> john_count;
    for (int i = 0; i < jobs_per; ++i) {
        mqpool.enqueue(john, [&, i]() {
            john_count.push_back(i);
        });
    }

    mqpool.pause(john);

    std::vector<int> paul_count;
    for (int i = 0; i < jobs_per; ++i) {
        mqpool.enqueue(paul, [&, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            paul_count.push_back(i);
        });
    }

    std::vector<int> george_count;
    for (int i = 0; i < jobs_per; ++i) {
        mqpool.enqueue(george, [&, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            george_count.push_back(i);
        });
    }

    QueueHandle ringo = mqpool.create_queue();

    std::vector<int> ringo_count;
    for (int i = 0; i < jobs_per; ++i) {
        mqpool.enqueue(ringo, [&, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ringo_count.push_back(i);
        });
    }

    mqpool.unpause(john);
    mqpool.flush_all();

    mqpool.destroy_queue(john);
    mqpool.destroy_queue(george);

    std::cout << "john:";
    for (const int i : john_count) {
        std::cout << ' ' << i;
    }
    std::cout << "\npaul:";
    for (const int i : paul_count) {
        std::cout << ' ' << i;
    }
    std::cout << "\ngeorge:";
    for (const int i : george_count) {
        std::cout << ' ' << i;
    }
    std::cout << "\nringo:";
    for (const int i : ringo_count) {
        std::cout << ' ' << i;
    }
    std::cout << '\n';
}
