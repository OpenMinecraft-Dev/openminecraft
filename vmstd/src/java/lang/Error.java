package java.lang;

public class Error extends Throwable
{
    public Error()
    {
        super();
    }

    public Error(String reason)
    {
        super(reason);
    }

    public Error(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public Error(Throwable cause)
    {
        super(cause);
    }
}
