package java.lang;

public class Class<T>
{
    private long nativePtr;
    private String name;

    Class(String name)
    {
        this.name = name;
    }

    public String getName()
    {
        return name;
    }
}