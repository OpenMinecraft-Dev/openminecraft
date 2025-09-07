package java.util.function;

import java.util.Objects;

public interface IntUnaryOperator {
    int applyAsInt(int operand);

    default IntUnaryOperator compose(IntUnaryOperator before) {
        Objects.requireNonNull(before);
        return (int v) -> applyAsInt(before.applyAsInt(v));
    }

    default IntUnaryOperator andThen(IntUnaryOperator after) {
        Objects.requireNonNull(after);
        return (int v) -> after.applyAsInt(applyAsInt(v));
    }

    static IntUnaryOperator identity() {
        return t -> t;
    }
}