#pragma once
#include <functional>

template <class T>
class linked_list
{
private:
    struct node
    {
        T *data;
        node *next;
    };
    node *root{nullptr};
    node *create_new_node(T *t, node *next)
    {
        node *new_node{new node{t, next}};
        return new_node;
    }

public:
    void add_to_start(T *t)
    {
        root = create_new_node(t, root);
    }

    void add_to_start(T &t)
    {
        add_to_start(&t);
    }

    void add_to_end(T *t)
    {
        if (root == nullptr)
            add_to_start(t);
        else
        {
            node *new_node{create_new_node(t, nullptr)}, *add_to{root};
            while (add_to->next != nullptr)
                add_to = add_to->next;
            add_to->next = new_node; //add to end of list
        }
    }

    void add_to_end(T &t)
    {
        add_to_end(&t);
    }

    unsigned short size()
    {
        unsigned short i{0};
        for (node *n = root; n != nullptr; n = n->next)
            i++;
        return i;
    }

    T *get(unsigned char index)
    {
        node *n{root};

        for (unsigned char i = 0; n != nullptr; i++)
        {
            if (i == index)
                return n->data;
            n = n->next;
        }
        return nullptr;
    }

    T *get_first()
    {
        return root == nullptr ? nullptr : root->data;
    }

    T *remove(unsigned char index)
    {
        if (index == 0)
            return remove_first();
        node *n{root}, *prev{nullptr};
        for (unsigned char i = 0; n != nullptr; i++)
        {
            if (i == index)
            {
                T *data{n->data};
                prev->next = n->next;
                delete n;
                return data;
            }
            prev = n;
            n = n->next;
        }
        return nullptr;
    }

    T *remove_first()
    {
        if (root == nullptr)
            return nullptr;
        node *n{root};
        T *data = root->data;
        root = n->next;
        delete n;
        return data;
    }

    T *remove_last()
    {
        if (root == nullptr)
            return nullptr;
        if (root->next == nullptr)
        {
            T *data{root->data};
            delete root;
            root = nullptr;
            return data;
        }
        node *n{root->next}, *prev{root};
        while (n != nullptr && n->next != nullptr)
        {
            prev = n;
            n = n->next;
        }
        T *data{n->data};
        prev->next = nullptr;
        delete n;
        return data;
    }

    void remove_if_true(std::function<bool(T *t)> execute, bool first_only = false)
    {
        for (node *n{root}, *prev{nullptr}; n != nullptr;)
        {
            if (n->data != nullptr && execute(n->data))
            {
                node *next = n->next;
                delete n;
                if (prev != nullptr)
                    prev->next = next;
                else
                    root = n->next;
                if (first_only)
                    return;
                n = next;
            }
            else
            {
                prev = n;
                n = n->next;
            }
        }
    }

    void execute_for_all(std::function<void(T *t)> execute)
    {
        for (node *n{root}; n != nullptr; n = n->next)
            if (n->data != nullptr)
                execute(n->data);
    }

    bool true_for_any(std::function<bool(T *t)> execute)
    {
        for (node *n{root}; n != nullptr; n = n->next)
            if (n->data != nullptr)
                if (execute(n->data))
                    return true;
        return false;
    }

    bool true_for_all(std::function<bool(T *t)> execute)
    {
        for (node *n{root}; n != nullptr; n = n->next)
            if (n->data != nullptr)
                if (!execute(n->data))
                    return false;
        return true;
    }

    void clear()
    {
        while (root != nullptr)
        {
            node *next{root->next};
            delete root;
            root = next;
        }
    }

    bool is_empty()
    {
        return root == nullptr;
    }
};