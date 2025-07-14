package java.lang;

public class String
{
    private byte[] data;
    public String(byte[] data)
    {
        this.data = data;
    }

    public String(String s)
    {
        data = s.data;
    }
}