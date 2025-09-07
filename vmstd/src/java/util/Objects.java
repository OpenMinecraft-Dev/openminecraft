package java.util;

public class Objects
{
    private Objects() {}

    public static <T> T requireNonNull(T obj) {
        if (obj == null)
        {
            throw new NullPointerException();
        }
        return obj;
    }

    public static boolean isNull(Object o) {
        return o == null;
    }
}