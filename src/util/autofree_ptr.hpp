#pragma once
#include <functional>

namespace horizon {
	template <typename T> class autofree_ptr {
	public:
		autofree_ptr(T *p, std::function<void(T*)> ffn): ptr(p), free_fn(ffn) {

		}
		autofree_ptr(std::function<void(T*)> ffn): free_fn(ffn) {

		}
		T *ptr = nullptr;
		std::function<void(T*)> free_fn;

		T& operator* () {
			return *ptr;
		}

		T* operator-> () const {
			return ptr;
		}

		operator T*() const {
			return ptr;
		}

		~autofree_ptr() {
			free_fn(ptr);
		}
	};
}
