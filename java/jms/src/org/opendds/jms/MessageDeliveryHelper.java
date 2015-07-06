/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

/**
 * @author  Steven Stallion
 */
public class MessageDeliveryHelper implements ConnectionStateListener {
    private ConnectionImpl connection;

    boolean locked;
    private Lock lock = new ReentrantLock();

    private boolean busy;
    private Condition busyCondition = lock.newCondition();

    private boolean start;
    private Condition startCondition = lock.newCondition();

    public MessageDeliveryHelper(ConnectionImpl connection) {
        this.connection = connection;
        connection.addStateListener(this);
    }

    public void connectionStarted(ConnectionStateEvent event) {
        lock();
        try {
            notifyStart();

        } finally {
            unlock();
        }
    }

    public void connectionStopped(ConnectionStateEvent event) {
        lock();
        try {
            awaitIdle();
            notifyStop();

        } catch (InterruptedException e) {
        } finally {
            unlock();
        }
    }

    public boolean isLocked() {
        return locked;
    }

    public void lock() {
        lock.lock();
        locked = true;
    }

    public void unlock() {
        lock.unlock();
        locked = false;
    }

    public void notifyStart() {
        checkLock();

        start = true;
        startCondition.signalAll();
    }

    public void notifyStop() {
        checkLock();

        start = false;
        startCondition.signalAll();
    }

    public void awaitStart() throws InterruptedException {
        checkLock();

        while (!start) {
            startCondition.await();
        }
    }

    public void notifyBusy() {
        checkLock();

        busy = true;
        busyCondition.signalAll();
    }

    public void notifyIdle() {
        checkLock();

        busy = false;
        busyCondition.signalAll();
    }

    public void awaitIdle() throws InterruptedException {
        checkLock();

        while (busy) {
            busyCondition.await();
        }
    }

    public void release() {
        connection.removeStateListener(this);
    }

    //

    private void checkLock() {
        if (!isLocked()) {
            throw new IllegalStateException("Lock not obtained; please call lock() first");
        }
    }
}
