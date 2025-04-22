#ifndef MINHEAP_H
#define MINHEAP_H

#include <vector>

template<typename T>
class MinHeap {
private:
    std::vector<T> heap;

    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heap[index] < heap[parent]) {
                std::swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(int index) {
        int size = heap.size();
        while (index < size) {
            int smallest = index;
            int left = 2 * index + 1;
            int right = 2 * index + 2;

            if (left < size && heap[left] < heap[smallest]) {
                smallest = left;
            }
            if (right < size && heap[right] < heap[smallest]) {
                smallest = right;
            }

            if (smallest != index) {
                std::swap(heap[index], heap[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }

public:
    MinHeap() {}

    void push(const T& value) {
        heap.push_back(value);
        heapifyUp(heap.size() - 1);
    }

    T top() const {
        if (heap.empty()) throw std::runtime_error("Heap is empty");
        return heap[0];
    }

    void pop() {
        if (heap.empty()) throw std::runtime_error("Heap is empty");
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) heapifyDown(0);
    }

    bool empty() const {
        return heap.empty();
    }

    int size() const {
        return heap.size();
    }
};

#endif