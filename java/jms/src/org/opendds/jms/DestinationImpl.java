package org.opendds.jms;

import javax.jms.Destination;

public class DestinationImpl implements Destination {
    private String destinationString;

    public DestinationImpl(String destinationString) {
        this.destinationString = destinationString;
    }

    public static Destination fromString(String s) {
        return new DestinationImpl(s);
    }

    public String toString() {
        return destinationString;
    }

    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        DestinationImpl that = (DestinationImpl) o;

        if (destinationString != null ? !destinationString.equals(that.destinationString) : that.destinationString != null)
            return false;

        return true;
    }

    public int hashCode() {
        return (destinationString != null ? destinationString.hashCode() : 0);
    }
}
