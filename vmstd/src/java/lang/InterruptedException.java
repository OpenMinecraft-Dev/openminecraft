package java.lang;

public class InterruptedException extends Exception
{
    public InterruptedException()
    {
        super();
    }

    public InterruptedException(String reason)
    {
        super(reason);
    }

    public InterruptedException(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public InterruptedException(Throwable cause)
    {
        super(cause);
    }
}
