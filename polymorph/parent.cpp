#include <iostream>
#include <vector>
#include <string>
#include <memory>

using namespace std;

struct document_t;
void draw(const document_t& doc, ostream& out, size_t position);

template <typename T>
void draw(const T& val, ostream& out, size_t position) {
	out << string(position, ' ') << val << '\n';
}

struct Foo {
};

void draw(const Foo& , ostream& out, size_t position) {
	out << string(position, ' ') << "Foo\n";
}
	
struct object_t {
	template <typename T>
	object_t(T val) : _self(make_shared<model<T>>(move(val))) {}
		
	friend void draw(const object_t& o, ostream& out, size_t position) {
		o._self->draw(out, position);
	}
	
private:
	struct concept_t {
		virtual void draw(ostream& out, size_t position) const = 0;
		virtual ~concept_t() noexcept = default;
	};
	
	template <typename T>
	struct model : concept_t {
		model(T val) : val(move(val)) {}
		
		void draw(ostream& out, size_t position) const override {
			::draw(val, out, position);
		}

	private:
		T val;
	};
	
	shared_ptr<const concept_t> _self;
};

struct document_t {
	
	void push_back(object_t val) {
		elem.push_back(val);
	}

	friend void draw(const document_t& doc, ostream& out, size_t position) {
		out << string(position, ' ') << "<document>\n";
		for (auto val : doc.elem) {
			draw(val, out, position+2);
		}
		out << string(position, ' ') << "</document>\n";
	}

private:
	std::vector<object_t> elem;
};



void main() {
	document_t doc;
	
	doc.push_back(0);
	doc.push_back(string("hello"));
	doc.push_back(doc);
	doc.push_back(Foo());
	doc.push_back(2.5);
	
	draw(doc, cout, 0);
}