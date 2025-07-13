package java.lang;

public class Object {
    public Object() {}
    public boolean equals(Object other) {
        return this == other;
    }

    public native int hashCode();
}
