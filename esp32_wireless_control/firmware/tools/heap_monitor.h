#ifndef HEAP_MONITOR_H
#define HEAP_MONITOR_H

#include <Arduino.h>

#include <uart.h>

/**
 * @brief Simple heap monitoring utility for ESP32
 * 
 * Use this to track heap usage and detect memory leaks.
 * Example usage:
 *   HeapMonitor::log("before-operation");
 *   doSomething();
 *   HeapMonitor::log("after-operation");
 */
class HeapMonitor
{
  public:
    /**
     * @brief Log current heap status with a tag
     * @param tag Label for this measurement point
     */
    static void log(const char* tag)
    {
#if DEBUG == 1
        size_t freeHeap = ESP.getFreeHeap();
        size_t minFreeHeap = ESP.getMinFreeHeap();
        size_t heapSize = ESP.getHeapSize();
        
        print_out("[HEAP:%s] Free: %u bytes | Min: %u bytes | Total: %u bytes | Used: %u bytes",
                  tag,
                  (unsigned)freeHeap,
                  (unsigned)minFreeHeap,
                  (unsigned)heapSize,
                  (unsigned)(heapSize - freeHeap));
#endif
    }

    /**
     * @brief Log heap delta between two points
     * @param tag Label for this measurement
     * @param previousFree Previous free heap value (pass by reference to update)
     */
    static void logDelta(const char* tag, size_t& previousFree)
    {
#if DEBUG == 1
        size_t currentFree = ESP.getFreeHeap();
        int32_t delta = (int32_t)currentFree - (int32_t)previousFree;
        
        print_out("[HEAP:%s] Free: %u bytes | Delta: %+d bytes",
                  tag,
                  (unsigned)currentFree,
                  (int)delta);
        
        previousFree = currentFree;
#endif
    }
};

#endif // HEAP_MONITOR_H
