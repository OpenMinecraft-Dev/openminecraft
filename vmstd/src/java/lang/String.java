package java.lang;

public final class String implements CharSequence, java.io.Serializable, Comparable<String>
{
    private byte[] value;
    private final byte coder;
    private int hash;
    private boolean hashIsZero;

    static final boolean COMPAT_STRINGS;

    static {
        COMPAT_STRINGS = true;
    }

    public String() {
        this.value = "".value;
        this.coder = "".coder;
    }

    public String(byte[] value)
    {
        this.value = value;
        coder = 0;
    }

    public String(String s)
    {
        value = s.value;
        coder = s.coder;
        hash = s.hash;
        hashIsZero = s.hashIsZero;
    }

    public int length()
    {
        return value.length;
    }

    public char charAt(int index)
    {
        return (char)value[index];
    }

    public CharSequence subSequence(int start, int end)
    {
        throw new RuntimeException("Not Implemented");
    }

    public String toString()
    {
        return this;
    }

    public int compare(String s)
    {
        return 0;
    }
}