Classfile /bridge/projects/openminecraft/vmstd/out/java/util/Collections$EmptyIterator.class
  Last modified 2026年1月21日; size 1257 bytes
  SHA-256 checksum 0997a8e4dc7d87a0c5031d010d620159bad43ad2bc84396a12423f01136d6c26
  Compiled from "Collections.java"
class java.util.Collections$EmptyIterator<E extends java.lang.Object> extends java.lang.Object implements java.util.Iterator<E>
  minor version: 0
  major version: 52
  flags: (0x0020) ACC_SUPER
  this_class: #8                          // java/util/Collections$EmptyIterator
  super_class: #10                        // java/lang/Object
  interfaces: 1, fields: 1, methods: 7, attributes: 3
Constant pool:
   #1 = Methodref          #8.#37         // java/util/Collections$EmptyIterator."<init>":()V
   #2 = Methodref          #10.#37        // java/lang/Object."<init>":()V
   #3 = Class              #38            // java/util/NoSuchElementException
   #4 = Methodref          #3.#37         // java/util/NoSuchElementException."<init>":()V
   #5 = Class              #39            // java/lang/IllegalStateException
   #6 = Methodref          #5.#37         // java/lang/IllegalStateException."<init>":()V
   #7 = Methodref          #40.#41        // java/util/Objects.requireNonNull:(Ljava/lang/Object;)Ljava/lang/Object;
   #8 = Class              #43            // java/util/Collections$EmptyIterator
   #9 = Fieldref           #8.#44         // java/util/Collections$EmptyIterator.EMPTY_ITERATOR:Ljava/util/Collections$EmptyIterator;
  #10 = Class              #45            // java/lang/Object
  #11 = Class              #46            // java/util/Iterator
  #12 = Utf8               EMPTY_ITERATOR
  #13 = Utf8               EmptyIterator
  #14 = Utf8               InnerClasses
  #15 = Utf8               Ljava/util/Collections$EmptyIterator;
  #16 = Utf8               Signature
  #17 = Utf8               Ljava/util/Collections$EmptyIterator<Ljava/lang/Object;>;
  #18 = Utf8               <init>
  #19 = Utf8               ()V
  #20 = Utf8               Code
  #21 = Utf8               LineNumberTable
  #22 = Utf8               hasNext
  #23 = Utf8               ()Z
  #24 = Utf8               next
  #25 = Utf8               ()Ljava/lang/Object;
  #26 = Utf8               ()TE;
  #27 = Utf8               remove
  #28 = Utf8               forEachRemaining
  #29 = Utf8               (Ljava/util/function/Consumer;)V
  #30 = Utf8               (Ljava/util/function/Consumer<-TE;>;)V
  #31 = Class              #47            // java/util/Collections$1
  #32 = Utf8               (Ljava/util/Collections$1;)V
  #33 = Utf8               <clinit>
  #34 = Utf8               <E:Ljava/lang/Object;>Ljava/lang/Object;Ljava/util/Iterator<TE;>;
  #35 = Utf8               SourceFile
  #36 = Utf8               Collections.java
  #37 = NameAndType        #18:#19        // "<init>":()V
  #38 = Utf8               java/util/NoSuchElementException
  #39 = Utf8               java/lang/IllegalStateException
  #40 = Class              #48            // java/util/Objects
  #41 = NameAndType        #49:#50        // requireNonNull:(Ljava/lang/Object;)Ljava/lang/Object;
  #42 = Class              #51            // java/util/Collections
  #43 = Utf8               java/util/Collections$EmptyIterator
  #44 = NameAndType        #12:#15        // EMPTY_ITERATOR:Ljava/util/Collections$EmptyIterator;
  #45 = Utf8               java/lang/Object
  #46 = Utf8               java/util/Iterator
  #47 = Utf8               java/util/Collections$1
  #48 = Utf8               java/util/Objects
  #49 = Utf8               requireNonNull
  #50 = Utf8               (Ljava/lang/Object;)Ljava/lang/Object;
  #51 = Utf8               java/util/Collections
{
  static final java.util.Collections$EmptyIterator<java.lang.Object> EMPTY_ITERATOR;
    descriptor: Ljava/util/Collections$EmptyIterator;
    flags: (0x0018) ACC_STATIC, ACC_FINAL
    Signature: #17                          // Ljava/util/Collections$EmptyIterator<Ljava/lang/Object;>;

  public boolean hasNext();
    descriptor: ()Z
    flags: (0x0001) ACC_PUBLIC
    Code:
      stack=1, locals=1, args_size=1
         0: iconst_0
         1: ireturn
      LineNumberTable:
        line 4190: 0

  public E next();
    descriptor: ()Ljava/lang/Object;
    flags: (0x0001) ACC_PUBLIC
    Code:
      stack=2, locals=1, args_size=1
         0: new           #3                  // class java/util/NoSuchElementException
         3: dup
         4: invokespecial #4                  // Method java/util/NoSuchElementException."<init>":()V
         7: athrow
      LineNumberTable:
        line 4191: 0
    Signature: #26                          // ()TE;

  public void remove();
    descriptor: ()V
    flags: (0x0001) ACC_PUBLIC
    Code:
      stack=2, locals=1, args_size=1
         0: new           #5                  // class java/lang/IllegalStateException
         3: dup
         4: invokespecial #6                  // Method java/lang/IllegalStateException."<init>":()V
         7: athrow
      LineNumberTable:
        line 4192: 0

  public void forEachRemaining(java.util.function.Consumer<? super E>);
    descriptor: (Ljava/util/function/Consumer;)V
    flags: (0x0001) ACC_PUBLIC
    Code:
      stack=1, locals=2, args_size=2
         0: aload_1
         1: invokestatic  #7                  // Method java/util/Objects.requireNonNull:(Ljava/lang/Object;)Ljava/lang/Object;
         4: pop
         5: return
      LineNumberTable:
        line 4195: 0
        line 4196: 5
    Signature: #30                          // (Ljava/util/function/Consumer<-TE;>;)V

  java.util.Collections$EmptyIterator(java.util.Collections$1);
    descriptor: (Ljava/util/Collections$1;)V
    flags: (0x1000) ACC_SYNTHETIC
    Code:
      stack=1, locals=2, args_size=2
         0: aload_0
         1: invokespecial #1                  // Method "<init>":()V
         4: return
      LineNumberTable:
        line 4186: 0

  static {};
    descriptor: ()V
    flags: (0x0008) ACC_STATIC
    Code:
      stack=2, locals=0, args_size=0
         0: new           #8                  // class java/util/Collections$EmptyIterator
         3: dup
         4: invokespecial #1                  // Method "<init>":()V
         7: putstatic     #9                  // Field EMPTY_ITERATOR:Ljava/util/Collections$EmptyIterator;
        10: return
      LineNumberTable:
        line 4187: 0
}
Signature: #34                          // <E:Ljava/lang/Object;>Ljava/lang/Object;Ljava/util/Iterator<TE;>;
SourceFile: "Collections.java"
InnerClasses:
  static #31;                             // class java/util/Collections$1
