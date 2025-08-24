package java.lang;

public class Exception extends Throwable
{
    public Exception()
    {
        super();
    }

    public Exception(String reason)
    {
        super(reason);
    }

    public Exception(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public Exception(Throwable cause)
    {
        super(cause);
    }
}