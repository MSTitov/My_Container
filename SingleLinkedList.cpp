#include <cassert>
#include <cstddef>
#include <iterator>
#include <vector>
#include <array>

using namespace std;

template <typename Type>
class SingleLinkedList {
    // Узел списка
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) {
        }
        Type value{};
        Node* next_node = nullptr;
    };

    // Шаблон класса Базовый Итератор.
    // Определяет поведение итератора на элементы односвязного списка
    // ValueType - совпадает с Type (для Iterator) либо с const Type (для ConstIterator)
    template <typename ValueType>
    class BasicIterator {
        // Класс списка объявляется дружественным, чтобы из методов списка
        // был доступ к приватной области итератора
        friend class SingleLinkedList;

        // Конвертирующий конструктор итератора из указателя на узел списка
        explicit BasicIterator(Node* node)
            : node_(node) {
        }

    public:
        // Объявленные ниже типы сообщают стандартной библиотеке о свойствах этого итератора

        // Категория итератора - forward iterator
        // (итератор, который поддерживает операции инкремента и многократное разыменование)
        using iterator_category = std::forward_iterator_tag;
        // Тип элементов, по которым перемещается итератор
        using value_type = Type;
        // Тип, используемый для хранения смещения между итераторами
        using difference_type = std::ptrdiff_t;
        // Тип указателя на итерируемое значение
        using pointer = ValueType*;
        // Тип ссылки на итерируемое значение
        using reference = ValueType&;

        BasicIterator() = default;

        // Конвертирующий конструктор/конструктор копирования
        // При ValueType, совпадающем с Type, играет роль копирующего конструктора
        // При ValueType, совпадающем с const Type, играет роль конвертирующего конструктора
        BasicIterator(const BasicIterator<Type>& other) noexcept
            : node_(other.node_) {
        }

        // Чтобы компилятор не выдавал предупреждение об отсутствии оператора = при наличии
        // пользовательского конструктора копирования, явно объявим оператор = и
        // попросим компилятор сгенерировать его за нас.
        BasicIterator& operator=(const BasicIterator& rhs) = default;

        // Оператор сравнения итераторов (в роли второго аргумента выступает константный итератор)
        // Два итератора равны, если они ссылаются на один и тот же элемент списка, либо на end()
        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return this->node_ == rhs.node_;
        }

        // Оператор, проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return !(*this == rhs);
        }

        // Оператор сравнения итераторов (в роли второго аргумента итератор)
        // Два итератора равны, если они ссылаются на один и тот же элемент списка, либо на end()
        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return this->node_ == rhs.node_;
        }

        // Оператор, проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return !(*this == rhs);
        }

        // Оператор прединкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает ссылку на самого себя
        // Инкремент итератора, не указывающего на существующий элемент списка, приводит к неопределённому поведению
        BasicIterator& operator++() noexcept {
            assert(node_ != nullptr);
            node_ = node_->next_node;
            return *this;
        }

        // Оператор постинкремента. После его вызова итератор указывает на следующий элемент списка.
        // Возвращает прежнее значение итератора
        // Инкремент итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        BasicIterator operator++(int) noexcept {
            auto this_copy(*this);
            ++(*this);
            return this_copy;
        }

        // Операция разыменования. Возвращает ссылку на текущий элемент
        // Вызов этого оператора, у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] reference operator*() const noexcept {
            assert(node_ != nullptr);
            return node_->value;
        }

        // Операция доступа к члену класса. Возвращает указатель на текущий элемент списка.
        // Вызов этого оператора, у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] pointer operator->() const noexcept {
            assert(node_ != nullptr);
            return &node_->value;
        }

    private:
        Node* node_ = nullptr;
    };

public:
    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;

    // Итератор, допускающий изменение элементов списка
    using Iterator = BasicIterator<Type>;
    // Константный итератор, предоставляющий доступ для чтения к элементам списка
    using ConstIterator = BasicIterator<const Type>;

    SingleLinkedList() = default;

    SingleLinkedList(std::initializer_list<Type> values) {
        Assign(values.begin(), values.end());  // Может бросить исключение
    }

    ~SingleLinkedList() {
        Clear();
    }

    // Возвращает итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    [[nodiscard]] Iterator begin() noexcept {
        return Iterator{ head_.next_node };
    }

    // Возвращает итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator end() noexcept {
        return {};
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    // Результат вызова эквивалентен вызову метода cbegin()
    [[nodiscard]] ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    // Результат вызова эквивалентен вызову метода cend()
    [[nodiscard]] ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен cend()
    [[nodiscard]] ConstIterator cbegin() const noexcept {
        return ConstIterator{ head_.next_node };
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator cend() const noexcept {
        return {};
    }

    // Возвращает итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator before_begin() noexcept {
        return Iterator{ &head_ };
    }

    // Возвращает константный итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return ConstIterator{ const_cast<Node*>(&head_) };
    }

    // Возвращает константный итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator before_begin() const noexcept {
        return cbefore_begin();
    }

    // Возвращает количество элементов в списке за время O(1)
    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    // Сообщает, пустой ли список за время O(1)
    [[nodiscard]] bool IsEmpty() const noexcept {
        return GetSize() == 0;
    }

    // Вставляет элемент value в начало списка за время O(1)
    void PushFront(const Type& value) {
        // Если оператор new выбросит исключение, указатель head_.next_node останется прежним
        head_.next_node = new Node(value, head_.next_node);

        // Последующий код не выбрасывает исключений
        ++size_;
    }

    /*
     * Вставляет элемент value после элемента, на который указывает pos.
     * Возвращает итератор на вставленный элемент
     * Если при создании элемента будет выброшено исключение, список останется в прежнем состоянии
     */
    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        assert(pos.node_ != nullptr);
        pos.node_->next_node = new Node(value, pos.node_->next_node);
        ++size_;
        return Iterator{ pos.node_->next_node };
    }

    void PopFront() noexcept {
        auto first_element = head_.next_node;
        head_.next_node = first_element->next_node;
        --size_;
        delete first_element;
    }

    /*
     * Удаляет элемент, следующий за pos.
     * Возвращает итератор на элемент, следующий за удалённым
     */
    Iterator EraseAfter(ConstIterator pos) noexcept {
        assert(pos.node_->next_node == nullptr);
        if (pos.node_->next_node == nullptr) {
            return {};
        }
        auto node_to_delete = pos.node_->next_node;
        pos.node_->next_node = node_to_delete->next_node;
        --size_;
        delete node_to_delete;
        return Iterator{ pos.node_->next_node };
    }

    // Очищает список за время O(N)
    void Clear() noexcept {
        // Удаляем элементы начиная с головы списка, пока они не закончатся
        while (head_.next_node != nullptr) {
            Node* next_node = head_.next_node->next_node;
            delete head_.next_node;
            head_.next_node = next_node;
        }
        size_ = 0;
    }

    // Обменивает содержимое списков за время O(1)
    void swap(SingleLinkedList& other) noexcept {
        std::swap(head_.next_node, other.head_.next_node);
        std::swap(size_, other.size_);
    }

private:
    template <typename InputIterator>
    void Assign(InputIterator from, InputIterator to) {
        // Создаём временный список, в который будем добавлять элементы из диапазона [from, to)
        // Если в процессе добавления будет выброшено исключение,
        // его деструктор подчистит всю память
        SingleLinkedList<Type> tmp;

        // Элементы будут записываться начиная с указателя на первый узел
        Node** node_ptr = &tmp.head_.next_node;
        while (from != to) {
            // Ожидается, что текущий указатель - нулевой
            assert(*node_ptr == nullptr);

            // Создаём новый узел и записываем его адрес в указатель текущего узла
            *node_ptr = new Node(*from, nullptr);
            ++tmp.size_;

            // Теперь node_ptr хранит адрес указателя на следующий узел
            node_ptr = &((*node_ptr)->next_node);

            // Переходим к следующему элементу диапазона
            ++from;
        }

        // Теперь, когда tmp содержит копию элементов диапазона [from, to),
        // можно совершить обмен данными текущего объекта и tmp
        swap(tmp);
        // Теперь текущий список содержит копию элементов диапазона [from, to),
        // а tmp - прежнее значение текущего списка
    }

    // Фиктивный узел, используется для вставки "перед первым элементом"
    Node head_;
    size_t size_ = 0;
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return (&lhs == &rhs)  // Оптимизация сравнения списка с собой
        || (lhs.GetSize() == rhs.GetSize()
            && std::equal(lhs.begin(), lhs.end(), rhs.begin()));  // может бросить исключение
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs == rhs);  // может бросить исключение
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());  // может бросить исключение
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(rhs < lhs);  // Может бросить исключение
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return (rhs < lhs);  // Может бросить исключение
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs < rhs);  // Может бросить исключение
}

bool IsPrime(int i) {
    for (int j = 2; j * j < i; ++j) {
        if (i % j == 0) {
            return false;
        }
    }
    return true;
}

// функция возвращает функциональный объект,
// выполняющий вставку в выбранный список
template<class Type>
auto PushToFunction(vector<SingleLinkedList<Type>>& lists, int x) {
    return [&](const Type& value) {
        lists[x].PushFront(value);
    };
}

template<class T>
void InsertRange(int from, int to, T push_function) {
    for (int i = from; i < to; ++i) {
        push_function(i);
    }
}

int main() {
    // main тестирует вектор, в нём нет ошибок.
    // не меняйте код этой функции
    // помимо этих тестов, список должен проходить все обычные тесты списка.
    // Ищите ошибки в коде списка и
    // других функциях этого файла

    vector<SingleLinkedList<int>> lists_a(10);

    auto push_to_2a = PushToFunction(lists_a, 2);
    auto push_to_5a = PushToFunction(lists_a, 5);
    auto push_to_7a = PushToFunction(lists_a, 7);

    InsertRange(10, 12, push_to_2a);
    InsertRange(12, 14, push_to_5a);
    InsertRange(14, 16, push_to_7a);

    assert(lists_a[2] == SingleLinkedList<int>({ 11, 10 }));
    assert(lists_a[5] == SingleLinkedList<int>({ 13, 12 }));
    assert(lists_a[7] == SingleLinkedList<int>({ 15, 14 }));

    vector<SingleLinkedList<int>> lists_b = lists_a;

    auto push_to_2b = PushToFunction(lists_b, 2);
    auto push_to_5b = PushToFunction(lists_b, 5);
    auto push_to_7b = PushToFunction(lists_b, 7);

    InsertRange(20, 22, push_to_2b);
    InsertRange(22, 24, push_to_5b);
    InsertRange(24, 26, push_to_7b);

    assert(lists_b[2] == SingleLinkedList<int>({ 21, 20, 11, 10 }));
    assert(lists_b[5] == SingleLinkedList<int>({ 23, 22, 13, 12 }));
    assert(lists_b[7] == SingleLinkedList<int>({ 25, 24, 15, 14 }));

    lists_a[2].PopFront();
    lists_a[5].InsertAfter(lists_a[5].begin(), 100);
    lists_b[5].EraseAfter(next(lists_b[5].begin()));
    lists_b[7].Clear();

    assert(lists_a[2] == SingleLinkedList<int>({ 10 }));
    assert(lists_a[5] == SingleLinkedList<int>({ 13, 100, 12 }));
    assert(lists_b[5] == SingleLinkedList<int>({ 23, 22, 12 }));
    assert(lists_b[7] == SingleLinkedList<int>());
}