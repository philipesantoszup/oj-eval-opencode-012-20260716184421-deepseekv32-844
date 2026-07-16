/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	struct Node {
		value_type* data;
		Node* next_in_bucket;
		Node* prev_in_bucket;
		Node* next_in_order;
		Node* prev_in_order;
		
		Node(value_type* val, Node* nb = nullptr, Node* pb = nullptr, Node* no = nullptr, Node* po = nullptr)
			: data(val), next_in_bucket(nb), prev_in_bucket(pb), next_in_order(no), prev_in_order(po) {}
		
		~Node() {
			delete data;
		}
	};

	Node** buckets;
	size_t bucket_count;
	size_t element_count;
	float load_factor_threshold;
	
	Node* head;  // dummy head for insertion order list
	Node* tail;  // dummy tail for insertion order list
	
	Hash hash_func;
	Equal equal_func;

	size_t get_bucket_index(const Key& key) const {
		return hash_func(key) % bucket_count;
	}

	void rehash(size_t new_bucket_count) {
		Node** new_buckets = new Node*[new_bucket_count]();
		for (size_t i = 0; i < new_bucket_count; ++i) {
			new_buckets[i] = nullptr;
		}
		
		Node* current = head->next_in_order;
		while (current != tail) {
			size_t new_index = hash_func(current->data->first) % new_bucket_count;
			Node*& bucket_head = new_buckets[new_index];
			
			current->next_in_bucket = bucket_head;
			if (bucket_head) bucket_head->prev_in_bucket = current;
			current->prev_in_bucket = nullptr;
			bucket_head = current;
			
			current = current->next_in_order;
		}
		
		delete[] buckets;
		buckets = new_buckets;
		bucket_count = new_bucket_count;
	}

	void ensure_capacity() {
		if (element_count >= bucket_count * load_factor_threshold) {
			size_t new_bucket_count = bucket_count * 2;
			if (new_bucket_count == 0) new_bucket_count = 8;
			rehash(new_bucket_count);
		}
	}

public:
 
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node* node_ptr;
		const linked_hashmap* container;
	public:
		// The following code is written for the C++ type_traits library.
		// Type traits is a C++ feature for describing certain properties of a type.
		// For instance, for an iterator, iterator::value_type is the type that the 
		// iterator points to. 
		// STL algorithms and containers may use these type_traits (e.g. the following 
		// typedef) to work properly. 
		// See these websites for more information:
		// https://en.cppreference.com/w/cpp/header/type_traits
		// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
		// About iterator_category: https://en.cppreference.com/w/cpp/iterator
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : node_ptr(nullptr), container(nullptr) {}
		iterator(Node* node, const linked_hashmap* cont) : node_ptr(node), container(cont) {}
		
		Node* get_node() const { return node_ptr; }
		iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			iterator temp = *this;
			node_ptr = node_ptr->next_in_order;
			return temp;
		}
		
		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->next_in_order;
			return *this;
		}
		
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			if (node_ptr == nullptr || node_ptr == container->head) {
				throw invalid_iterator();
			}
			iterator temp = *this;
			node_ptr = node_ptr->prev_in_order;
			return temp;
		}
		
		/**
		 * TODO --iter
		 */
		iterator & operator--() {
			if (node_ptr == nullptr || node_ptr == container->head) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->prev_in_order;
			return *this;
		}
		
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			if (node_ptr == nullptr || node_ptr == container->head || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			return *(node_ptr->data);
		}
		
		bool operator==(const iterator &rhs) const {
			return node_ptr == rhs.node_ptr;
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node_ptr == rhs.node_ptr;
		}
		
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return node_ptr != rhs.node_ptr;
		}
		
		bool operator!=(const const_iterator &rhs) const {
			return node_ptr != rhs.node_ptr;
		}

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const {
			if (node_ptr == nullptr || node_ptr == container->head || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			return node_ptr->data;
		}
		
		friend class linked_hashmap;
		friend class const_iterator;
	};
 
	class const_iterator {
	private:
		const Node* node_ptr;
		const linked_hashmap* container;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = const value_type*;
		using reference = const value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node_ptr(nullptr), container(nullptr) {}
		const_iterator(const Node* node, const linked_hashmap* cont) : node_ptr(node), container(cont) {}
		const_iterator(const const_iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		const_iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		
		const_iterator operator++(int) {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			const_iterator temp = *this;
			node_ptr = node_ptr->next_in_order;
			return temp;
		}
		
		const_iterator & operator++() {
			if (node_ptr == nullptr || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->next_in_order;
			return *this;
		}
		
		const_iterator operator--(int) {
			if (node_ptr == nullptr || node_ptr == container->head) {
				throw invalid_iterator();
			}
			const_iterator temp = *this;
			node_ptr = node_ptr->prev_in_order;
			return temp;
		}
		
		const_iterator & operator--() {
			if (node_ptr == nullptr || node_ptr == container->head) {
				throw invalid_iterator();
			}
			node_ptr = node_ptr->prev_in_order;
			return *this;
		}
		
		const value_type & operator*() const {
			if (node_ptr == nullptr || node_ptr == container->head || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			return *(node_ptr->data);
		}
		
		bool operator==(const iterator &rhs) const {
			return node_ptr == rhs.node_ptr;
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node_ptr == rhs.node_ptr;
		}
		
		bool operator!=(const iterator &rhs) const {
			return node_ptr != rhs.node_ptr;
		}
		
		bool operator!=(const const_iterator &rhs) const {
			return node_ptr != rhs.node_ptr;
		}
		
		const value_type* operator->() const {
			if (node_ptr == nullptr || node_ptr == container->head || node_ptr == container->tail) {
				throw invalid_iterator();
			}
			return node_ptr->data;
		}
		
		friend class linked_hashmap;
		friend class iterator;
	};
 
	/**
	 * TODO two constructors
	 */
	linked_hashmap() : bucket_count(8), element_count(0), load_factor_threshold(0.75f) {
		buckets = new Node*[bucket_count]();
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		
		head = new Node(nullptr);
		tail = new Node(nullptr);
		head->next_in_order = tail;
		tail->prev_in_order = head;
	}
	
	linked_hashmap(const linked_hashmap &other) : bucket_count(other.bucket_count), 
	                                             element_count(0), 
	                                             load_factor_threshold(other.load_factor_threshold) {
		buckets = new Node*[bucket_count]();
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		
		head = new Node(nullptr);
		tail = new Node(nullptr);
		head->next_in_order = tail;
		tail->prev_in_order = head;
		
		const Node* current = other.head->next_in_order;
		while (current != other.tail) {
			insert(*(current->data));
			current = current->next_in_order;
		}
	}
 
	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		
		clear();
		
		delete[] buckets;
		delete head;
		delete tail;
		
		bucket_count = other.bucket_count;
		element_count = 0;
		load_factor_threshold = other.load_factor_threshold;
		
		buckets = new Node*[bucket_count]();
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		
		head = new Node(nullptr);
		tail = new Node(nullptr);
		head->next_in_order = tail;
		tail->prev_in_order = head;
		
		const Node* current = other.head->next_in_order;
		while (current != other.tail) {
			insert(*(current->data));
			current = current->next_in_order;
		}
		
		return *this;
	}
 
	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
		clear();
		delete[] buckets;
		delete head;
		delete tail;
	}
 
	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		iterator it = find(key);
		if (it == end()) {
			throw index_out_of_bound();
		}
		return it->second;
	}
	
	const T & at(const Key &key) const {
		const_iterator it = find(key);
		if (it == cend()) {
			throw index_out_of_bound();
		}
		return it->second;
	}
 
	/**
	 * TODO
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		iterator it = find(key);
		if (it != end()) {
			return it->second;
		}
		T default_value = T();
		pair<iterator, bool> result = insert(value_type(key, default_value));
		return result.first->second;
	}
 
	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}
 
	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		return iterator(head->next_in_order, this);
	}
	
	const_iterator cbegin() const {
		return const_iterator(head->next_in_order, this);
	}
 
	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(tail, this);
	}
	
	const_iterator cend() const {
		return const_iterator(tail, this);
	}
 
	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return element_count == 0;
	}
 
	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return element_count;
	}
 
	/**
	 * clears the contents
	 */
	void clear() {
		Node* current = head->next_in_order;
		while (current != tail) {
			Node* next = current->next_in_order;
			delete current;
			current = next;
		}
		
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		
		head->next_in_order = tail;
		tail->prev_in_order = head;
		element_count = 0;
	}
 
	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		iterator it = find(value.first);
		if (it != end()) {
			return pair<iterator, bool>(it, false);
		}
		
		ensure_capacity();
		
		size_t bucket_index = get_bucket_index(value.first);
		Node* new_node = new Node(new value_type(value));
		
		// Add to bucket
		new_node->next_in_bucket = buckets[bucket_index];
		if (buckets[bucket_index]) buckets[bucket_index]->prev_in_bucket = new_node;
		new_node->prev_in_bucket = nullptr;
		buckets[bucket_index] = new_node;
		
		// Add to insertion order list (at the end)
		new_node->prev_in_order = tail->prev_in_order;
		new_node->next_in_order = tail;
		tail->prev_in_order->next_in_order = new_node;
		tail->prev_in_order = new_node;
		
		element_count++;
		return pair<iterator, bool>(iterator(new_node, this), true);
	}
 
	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos == end() || pos.container != this) {
			throw invalid_iterator();
		}
		
		Node* node = pos.node_ptr;
		if (node == head || node == tail) {
			throw invalid_iterator();
		}
		
		// Remove from bucket
		size_t bucket_index = get_bucket_index(node->data->first);
		if (node->prev_in_bucket) {
			node->prev_in_bucket->next_in_bucket = node->next_in_bucket;
		} else {
			buckets[bucket_index] = node->next_in_bucket;
		}
		if (node->next_in_bucket) {
			node->next_in_bucket->prev_in_bucket = node->prev_in_bucket;
		}
		
		// Remove from insertion order list
		node->prev_in_order->next_in_order = node->next_in_order;
		node->next_in_order->prev_in_order = node->prev_in_order;
		
		delete node;
		element_count--;
	}
 
	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
		return find(key) != cend() ? 1 : 0;
	}
 
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		size_t bucket_index = get_bucket_index(key);
		Node* current = buckets[bucket_index];
		
		while (current) {
			if (equal_func(current->data->first, key)) {
				return iterator(current, this);
			}
			current = current->next_in_bucket;
		}
		
		return end();
	}
	
	const_iterator find(const Key &key) const {
		size_t bucket_index = get_bucket_index(key);
		const Node* current = buckets[bucket_index];
		
		while (current) {
			if (equal_func(current->data->first, key)) {
				return const_iterator(current, this);
			}
			current = current->next_in_bucket;
		}
		
		return cend();
	}
};

}

#endif
