#include <iterator>
#include <stack>

#include "interfaces/Scene.hpp"

class DFIterator {
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = ENG::Node*;
	using difference_type = std::ptrdiff_t;
	using pointer = ENG::Node**;
	using reference = ENG::Node*&;

	DFIterator() = default;
	explicit DFIterator(ENG::Node* root);
	ENG::Node* operator*() const;
	DFIterator& operator++();
	bool operator!=(const DFIterator& other) const;

private:
	struct Frame {
		ENG::Node* node;
		size_t childIndex;
		bool operator==(const Frame& other) const {
			return node == other.node && childIndex == other.childIndex;
		}
	};

	std::stack<Frame> stack;
};

class DFTraversal {
public:
	explicit DFTraversal(ENG::Node* root) : root(root) {}
	DFIterator begin() const;
	DFIterator end() const;

private:
	ENG::Node* root;
};
