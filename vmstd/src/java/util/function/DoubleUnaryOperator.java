package java.util.function;

import java.util.Objects;

public interface DoubleUnaryOperator {
    double applyAsDouble(double operand);

    default DoubleUnaryOperator compose(DoubleUnaryOperator before) {
        Objects.requireNonNull(before);
        return (double v) -> applyAsDouble(before.applyAsDouble(v));
    }

    default DoubleUnaryOperator andThen(DoubleUnaryOperator after) {
        Objects.requireNonNull(after);
        return (double v) -> after.applyAsDouble(applyAsDouble(v));
    }

    static DoubleUnaryOperator identity() {
        return t -> t;
    }
}