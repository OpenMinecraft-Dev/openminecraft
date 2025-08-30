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
}