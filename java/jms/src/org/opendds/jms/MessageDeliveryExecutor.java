/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * @author  Steven Stallion
 */
public class MessageDeliveryExecutor implements Executor {
    private BlockingQueue<Runnable> commands =
        new LinkedBlockingQueue<Runnable>();

    private MessageDeliveryHelper helper;
    private Thread worker;

    public MessageDeliveryExecutor(ConnectionImpl connection) {
        this.helper = new MessageDeliveryHelper(connection);
        this.worker = new Thread() {
            @Override
            public void run() {
                try {
                    while (!isInterrupted()) {
                        helper.lock();
                        try {
                            helper.awaitStart();
                            helper.notifyBusy();

                            Runnable command = commands.take();
                            command.run();

                        } finally {
                            helper.notifyIdle();
                            helper.unlock();
                        }
                    }

                } catch (InterruptedException e) {}
            }
        };
        worker.start();
    }

    public void execute(Runnable command) {
        commands.add(command);
    }

    public void shutdown() {
        if (worker == null) {
            return; // ignore
        }

        worker.interrupt();
        try {
            worker.join();
            worker = null;

        } catch (InterruptedException e) {
            throw new IllegalStateException("Unable to join() on worker thread; possible resource leak");
        }

        helper.release();
    }
}
