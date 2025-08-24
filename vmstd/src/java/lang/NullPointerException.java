package java.lang;

public class NullPointerException extends RuntimeException
{
    public NullPointerException()
    {
        super();
    }

    public NullPointerException(String reason)
    {
        super(reason);
    }

    public NullPointerException(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public NullPointerException(Throwable cause)
    {
        super(cause);
    }
}