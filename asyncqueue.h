#ifndef __ASYNCQUEUE_H
#define __ASYNCQUEUE_H

#include <QMutex>
#include <QWaitCondition>
#include <QQueue>

template<class T>
class AsyncQueue {
public:
    int size() {
        QMutexLocker locker(&mutex);
        return buffer.size();
    }

    void put(const T& item) {
        QMutexLocker locker(&mutex);
        buffer.enqueue(item);

        if (queue > 0) {
            empty.wakeAll();
        }
    }

    T get() {
        QMutexLocker locker(&mutex);
        while (buffer.isEmpty()) {
            queue++;
            empty.wait(&mutex);
            queue--;
        }

        return buffer.dequeue();
    }

private:
    QWaitCondition empty;
    QMutex mutex;
    QQueue<T> buffer;
    short queue = 0;
};


#endif // __ASYNCQUEUE_H
