#include<iterator>
#include<stack>

#include "algorithms/DFT.hpp"
#include "interfaces/Scene.hpp"

using iterator_category = std::forward_iterator_tag;
using value_type = ENG::Node*;
using difference_type = std::ptrdiff_t;
using pointer = ENG::Node**;
using reference = ENG::Node*&;

DFIterator::DFIterator(ENG::Node* root) {
	if (root) stack.emplace(Frame{ root, 0 });
}

ENG::Node* DFIterator::operator*() const {
	return stack.empty() ? nullptr : stack.top().node;
}

DFIterator& DFIterator::operator++() {
	if (stack.empty()) return *this;

	Frame& top = stack.top();
	ENG::Node* current = top.node;

	if (top.childIndex < current->children.size()) {
		ENG::Node* child = current->children[top.childIndex++];
		stack.emplace(Frame{ child, 0 });
	}
	else {
		stack.pop();
		if (!stack.empty()) ++(*this); // Continue to next sibling or backtrack
	}

	return *this;
}

bool DFIterator::operator!=(const DFIterator& other) const {
	return stack != other.stack;
}

DFIterator DFTraversal::begin() const 
{ 
	return DFIterator(root); 
}

DFIterator DFTraversal::end() const 
{ 
	return DFIterator(); 
}
