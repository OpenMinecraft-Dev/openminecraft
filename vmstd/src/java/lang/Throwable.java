package java.lang;

public class Throwable
{
    private String reason;
    private Throwable cause;

    public Throwable()
    {
    }

    public Throwable(String reason)
    {
        this.reason = reason;
    }
}