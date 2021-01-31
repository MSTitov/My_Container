
#include <cassert>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <numeric>

using namespace std;

template <typename Type>
class ArrayPtr {
public:
    // Инициализирует ArrayPtr пустым указателем
    ArrayPtr() = default;

    // Создаёт в куче массив из size элементов типа Type.
    // Если size == 0, поле raw_ptr_ должно быть равно nullptr
    explicit ArrayPtr(size_t size)
        : raw_ptr_(size != 0 ? new Type[size] : nullptr) {
    }

    // Конструктор из сырого указателя, хранящего адрес массива в куче либо nullptr
    explicit ArrayPtr(Type* raw_ptr) noexcept
        : raw_ptr_(raw_ptr) {
    }

    // Запрещаем копирование
    ArrayPtr(const ArrayPtr&) = delete;

    ~ArrayPtr() {
        delete[] raw_ptr_;
    }

    // Запрещаем присваивание
    ArrayPtr& operator=(const ArrayPtr&) = delete;

    // Прекращает владением массивом в памяти, возвращает значение адреса массива
    // После вызова метода указатель на массив должен стать обнулиться
    [[nodiscard]] Type* Release() noexcept {
        Type* p = raw_ptr_;
        raw_ptr_ = nullptr;
        return p;
    }

    // Возвращает ссылку на элемент массива с индексом index
    Type& operator[](size_t index) noexcept {
        assert(raw_ptr_);
        return raw_ptr_[index];
    }

    // Возвращает константную ссылку на элемент массива с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(raw_ptr_);
        return raw_ptr_[index];
    }

    // Возвращает true, если указатель ненулевой, и false в противном случае
    explicit operator bool() const {
        return raw_ptr_ != nullptr;
    }

    // Возвращает значение сырого указателя, хранящего адрес начала массива
    Type* Get() const noexcept {
        return raw_ptr_;
    }

    // Обменивается значениям указателя на массив с объектом other
    void swap(ArrayPtr& other) noexcept {
        std::swap(raw_ptr_, other.raw_ptr_);
    }

private:
    Type* raw_ptr_ = nullptr;
};

struct ReserveProxyObj {
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : capacity(capacity_to_reserve) {
    }
    size_t capacity;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
    using ItemsPtr = ArrayPtr<Type>;

public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type{})  // Делегируем инициализацию конструктору, принимающему size и value
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : items_(size)  // может бросить исключение
        , size_(size)
        , capacity_(size)  //
    {
        std::fill(items_.Get(), items_.Get() + size_, value);  // Может бросить исключение
    }

    // Создаёт вектор из initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size())  // Может бросить исключение
        , size_(init.size())
        , capacity_(init.size())  //
    {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(ReserveProxyObj reserved)
        : items_(reserved.capacity)
        , size_(0)
        , capacity_(reserved.capacity) {
    }

    SimpleVector(const SimpleVector& other)
        : items_(other.size_)  // может бросить исключение
        , size_(other.size_)
        , capacity_(other.size_)  //
    {
        std::copy(other.items_.Get(), other.items_.Get() + size_, items_.Get());  // может бросить исключение
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&rhs != this) {  // оптимизация присваивания вектора самому себе
            if (rhs.IsEmpty()) {
                // Оптимизация для случая присваивания пустого вектора
                Clear();
            }
            else {
                // Применяем идиому Copy-and-swap
                SimpleVector rhs_copy(rhs);  // может бросить исключение
                swap(rhs_copy);
            }
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) noexcept
        : items_(other.size_)  // может бросить исключение
        , size_(other.size_)
        , capacity_(other.size_)  //
    {
        std::copy(std::make_move_iterator(other.items_.Get()), std::make_move_iterator(other.items_.Get() + size_), items_.Get());
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept
    {
        if (&rhs != this)
        {
            if (rhs.IsEmpty()) {
                // Оптимизация для случая присваивания пустого вектора
                Clear();
            }
            else {
                size_ = rhs.size_;
                capacity_ = rhs.capacity_;
                rhs.size_ = 0;
                rhs.capacity_ = 0;
            }
        }
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            using namespace std;
            throw out_of_range("Item index is out of range"s);
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            using namespace std;
            throw out_of_range("Item index is out of range"s);
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            auto new_items = ReallocateCopy(new_capacity);  // может бросить исключение

            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            // Вычисляем вместимость вектора
            const size_t new_capacity = std::max(capacity_ * 2, new_size);

            // Копируем существующие элементы вектора на новое местое
            auto new_items = ReallocateCopy(new_capacity);  // может бросить исключение
            // заполняем добавленные элементы значением по умолчанию
            std::fill(new_items.Get() + size_, new_items.Get() + new_size, std::move(Type{}));  // может бросить исключение

            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        else if (new_size > size_) {
            assert(new_size <= capacity_);
            std::fill(items_.Get() + size_, items_.Get() + new_size, Type{});  // может бросить исключение
        }
        size_ = new_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type item) {
        const size_t new_size = size_ + 1;
        if (new_size > capacity_) {
            const size_t new_capacity = std::max(capacity_ * 2, new_size);

            auto new_items = ReallocateCopy(new_capacity);  // может бросить исключение
            new_items.Get()[size_] = std::move(item);                  // может бросить исключение

            capacity_ = new_capacity;
            items_.swap(new_items);
        }
        else {
            items_.Get()[size_] = std::move(item);
        }
        size_ = new_size;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(Iterator pos, Type value) {
        assert(begin() <= pos && pos <= end());
        size_t new_size = size_ + 1;
        size_t new_item_offset = pos - cbegin();
        if (new_size <= capacity_) {  // Вместимость вектора достаточна для вставки элемента
            Iterator mutable_pos = begin() + new_item_offset;

            // Копируем элементы, начиная с последнего, чтобы перенести "хвост" вправо
            std::copy_backward(std::make_move_iterator(pos), std::make_move_iterator(end()),
                end() + 1);  // может выбросить исключение
            *mutable_pos = std::move(value);           // может выбросить исключение

        }
        else {  // Требуется перевыделить память
            size_t new_capacity = std::max(capacity_ * 2, new_size);

            ItemsPtr new_items(new_capacity);  // может выбросить исключение
            Iterator new_items_pos = new_items.Get() + new_item_offset;

            // Копируем элементы, предшествующие вставляемому
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(pos),
                new_items.Get());  // может выбросить исключение
// Вставляем элемент в позицию вставки
            *new_items_pos = std::move(value);  // может выбросить исключение
            // Копируем элементы следующие за вставляемым
            std::copy(std::make_move_iterator(pos), std::make_move_iterator(end()),
                new_items_pos + 1);  // может выбросить исключение

            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        size_ = new_size;
        return begin() + new_item_offset;
    }

    // "Удаляет" элемент с конца вектора, не уменьшая его вместимость
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в позиции pos, сдвигая следующие элементы на его место.
    // Итератор pos должен быть итератором, ссылающимся на существующий элемент вектора.
    // Возвращает итератор на элемент, который следует за последним удалённым элементом.
    // Если был удалён последний элемент, должен вернуться итератор end().
    Iterator Erase(Iterator pos) {
        assert(begin() <= pos && pos < end());
        Iterator mutable_pos = begin() + (pos - begin());

        // Переносим "хвост" влево на места удаляемого элемента
        std::copy(std::make_move_iterator(mutable_pos + 1), std::make_move_iterator(end()),
            mutable_pos);  // может выбросить исключение

        --size_;
        return mutable_pos;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    // Выделяет копию текущего массива с заданной вместимостью
    ItemsPtr ReallocateCopy(size_t new_capacity) const {
        ItemsPtr new_items(new_capacity);  // может бросить исключение
        size_t copy_size = std::min(new_capacity, size_);
        std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + copy_size),
            new_items.Get());  // может бросить исключение
        return ItemsPtr(new_items.Release());
    }

    ItemsPtr items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs.GetSize() == rhs.GetSize())
        && std::equal(lhs.begin(), lhs.end(), rhs.begin());  // может бросить исключение
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);  // может бросить исключение
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());  // может бросить исключение
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);  // может бросить исключение
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;  // может бросить исключение
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs <= lhs;  // может бросить исключение
}

class X {
public:
    X()
        : X(5) {
    }
    X(size_t num)
        : x_(num) {
    }
    X(const X& other) = delete;
    X& operator=(const X& other) = delete;
    X(X&& other) {
        x_ = exchange(other.x_, 0);
    }
    X& operator=(X&& other) {
        x_ = exchange(other.x_, 0);
        return *this;
    }
    size_t GetX() const {
        return x_;
    }

private:
    size_t x_;
};

SimpleVector<int> GenerateVector(size_t size) {
    SimpleVector<int> v(size);
    iota(v.begin(), v.end(), 1);
    return v;
}

void TestTemporaryObjConstructor() {
    const size_t size = 1000000;
    cout << "Test with temporary object, copy elision" << endl;
    SimpleVector<int> moved_vector(GenerateVector(size));
    assert(moved_vector.GetSize() == size);
    cout << "Done!" << endl << endl;
}

void TestTemporaryObjOperator() {
    const size_t size = 1000000;
    cout << "Test with temporary object, operator=" << endl;
    SimpleVector<int> moved_vector;
    assert(moved_vector.GetSize() == 0);
    moved_vector = GenerateVector(size);
    assert(moved_vector.GetSize() == size);
    cout << "Done!" << endl << endl;
}

void TestNamedMoveConstructor() {
    const size_t size = 1000000;
    cout << "Test with named object, move constructor" << endl;
    SimpleVector<int> vector_to_move(GenerateVector(size));
    assert(vector_to_move.GetSize() == size);

    SimpleVector<int> moved_vector(move(vector_to_move));
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);
    cout << "Done!" << endl << endl;
}

void TestNamedMoveOperator() {
    const size_t size = 1000000;
    cout << "Test with named object, operator=" << endl;
    SimpleVector<int> vector_to_move(GenerateVector(size));
    assert(vector_to_move.GetSize() == size);

    SimpleVector<int> moved_vector = move(vector_to_move);
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);
    cout << "Done!" << endl << endl;
}

void TestNoncopiableMoveConstructor() {
    const size_t size = 5;
    cout << "Test noncopiable object, move constructor" << endl;
    SimpleVector<X> vector_to_move;
    for (size_t i = 0; i < size; ++i) {
        vector_to_move.PushBack(X(i));
    }

    SimpleVector<X> moved_vector = move(vector_to_move);
    assert(moved_vector.GetSize() == size);
    assert(vector_to_move.GetSize() == 0);

    for (size_t i = 0; i < size; ++i) {
        assert(moved_vector[i].GetX() == i);
    }
    cout << "Done!" << endl << endl;
}

void TestNoncopiablePushBack() {
    const size_t size = 5;
    cout << "Test noncopiable push back" << endl;
    SimpleVector<X> v;
    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    assert(v.GetSize() == size);

    for (size_t i = 0; i < size; ++i) {
        assert(v[i].GetX() == i);
    }
    cout << "Done!" << endl << endl;
}

void TestNoncopiableInsert() {
    const size_t size = 5;
    cout << "Test noncopiable insert" << endl;
    SimpleVector<X> v;
    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    // в начало
    v.Insert(v.begin(), X(size + 1));
    assert(v.GetSize() == size + 1);
    assert(v.begin()->GetX() == size + 1);
    // в конец
    v.Insert(v.end(), X(size + 2));
    assert(v.GetSize() == size + 2);
    assert((v.end() - 1)->GetX() == size + 2);
    // в середину
    v.Insert(v.begin() + 3, X(size + 3));
    assert(v.GetSize() == size + 3);
    assert((v.begin() + 3)->GetX() == size + 3);
    cout << "Done!" << endl << endl;
}

void TestNoncopiableErase() {
    const size_t size = 3;
    cout << "Test noncopiable erase" << endl;
    SimpleVector<X> v;
    for (size_t i = 0; i < size; ++i) {
        v.PushBack(X(i));
    }

    auto it = v.Erase(v.begin());
    assert(it->GetX() == 1);
    cout << "Done!" << endl << endl;
}

void TestResizeMethodNoCopy() {
    cout << "TestResizeMethodNoCopy"s << endl;
    // создаем пустой вектор элементов с запрещённым копированием
    SimpleVector<X> v;

    // поместим 10 элементов в вектор
    for (int i = 0; i < 10; ++i) {
        v.PushBack(X(i));
    }
    assert(v.GetSize() == 10);

    // увеличиваем размер в два раза
    v.Resize(20);

    assert(v.GetSize() == 20);

    // проверяем, что первые 10 элементов не изменились
    for (size_t i = 0; i < 10; ++i) {
        assert(v[i].GetX() == i);
    }

    // проверяем, что следующие 10 элементов заполнились значением по-умолчанию
    for (size_t i = 10; i < 20; ++i) {
        assert(v[i].GetX() == 5);
    }

    // уменьшаем размер в 4 раза
    v.Resize(5);

    assert(v.GetSize() == 5);

    // проверяем, что первые 5 элементов не изменились
    for (size_t i = 0; i < 5; ++i) {
        assert(v[i].GetX() == i);
    }

    // увеличиваем размер в два раза
    v.Resize(10);

    assert(v.GetSize() == 10);

    // проверяем, что первые 5 элементов не изменились
    for (size_t i = 0; i < 5; ++i) {
        assert(v[i].GetX() == i);
    }

    // проверяем, что следующие 5 элементов заполнились значением по-умолчанию
    for (size_t i = 5; i < 10; ++i) {
        assert(v[i].GetX() == 5);
    }
    cout << "Done!"s << endl << endl;
}

int main() {
    TestTemporaryObjConstructor();
    TestTemporaryObjOperator();
    TestNamedMoveConstructor();
    TestNamedMoveOperator();
    TestNoncopiableMoveConstructor();
    TestNoncopiablePushBack();
    TestNoncopiableInsert();
    TestNoncopiableErase();
    TestResizeMethodNoCopy();
    return 0;
}