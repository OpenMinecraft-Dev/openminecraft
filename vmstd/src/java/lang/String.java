package java.lang;

public class String implements CharSequence
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

    public int length()
    {
        return data.length;
    }

    public char charAt(int index)
    {
        return (char)data[index];
    }

    public CharSequence subSequence(int start, int end)
    {
        throw new RuntimeException("Not Implemented");
    }

    public String toString()
    {
        return this;
    }
}