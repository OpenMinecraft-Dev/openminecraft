package java.lang.invoke;

public class LambdaMetafactory {
    public static CallSite metafactory(MethodHandles.Lookup caller, String interfaceMethodName, MethodType factoryType, MethodType interfaceMethodType, MethodHandle implementation, MethodType dynamicMethodType) {
        return new CallSite() {};
    }
}
