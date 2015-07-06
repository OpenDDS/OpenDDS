/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.esb.helpers;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * @author  Steven Stallion
 */
public class ThreadedQueue<T> {
    public static final int MIN_THREADS = 1;

    public static interface QueueListener<T> {
        void dequeue(T t);
    }

    private class QueueThread extends Thread {
        @Override
        public void run() {
            try {
                while (!interrupted()) {
                    listener.dequeue(queue.take());
                }

            } catch (InterruptedException e) {}
        }
    }

    private QueueListener<T> listener;

    private List<Thread> threads =
        new LinkedList<Thread>();

    private BlockingQueue<T> queue =
        new LinkedBlockingQueue<T>();

    public ThreadedQueue(QueueListener<T> listener) {
        this(listener, MIN_THREADS);
    }

    public ThreadedQueue(QueueListener<T> listener, int maxThreads) {
        assert listener != null;

        this.listener = listener;

        int numThreads = Math.max(MIN_THREADS, maxThreads);
        for (int i = 0; i < numThreads; ++i) {
            threads.add(new QueueThread());
        }
    }

    public int numberOfThreads() {
        return threads.size();
    }

    public synchronized void start() {
        // Start queue threads
        issue(new ThreadCommand() {
            public void execute(Thread t) throws Exception {
                t.start();
            }
        });
    }

    public boolean isEmpty() {
        return queue.isEmpty();
    }

    public int size() {
        return queue.size();
    }

    public void enqueue(T t) {
        try {
            queue.put(t);

        } catch (InterruptedException e) {
            throw new IllegalStateException(e);
        }
    }

    public synchronized void shutdown() {
        // Interrupt running threads (cooperative shutdown)
        issue(new ThreadCommand() {
            public void execute(Thread t) throws Exception {
                if (t.isAlive()) {
                    t.interrupt();
                }
            }
        });

        // Wait for running threads to terminate
        issue(new ThreadCommand() {
            public void execute(Thread t) throws Exception {
                if (t.isAlive()) {
                    t.join();
                }
            }
        });
    }

    //

    private static interface ThreadCommand {
        void execute(Thread t) throws Exception;
    }

    private void issue(ThreadCommand command) {
        try {
            for (Thread t : threads) {
                command.execute(t);
            }

        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }
}
