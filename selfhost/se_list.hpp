#pragma once
#include <vector>
#include <memory>
#include <initializer_list>
#include <utility>
#include <cstddef>
#include <cstdint>

// SE collection assignment copies a reference to the collection. Explicit
// copying operations can clone the outer storage while retaining nested values.
template<class T> class SEList {
  using Storage=std::vector<T>;
  std::shared_ptr<Storage> storage_;
public:
  using value_type=T;
  using iterator=typename Storage::iterator;
  using const_iterator=typename Storage::const_iterator;
  using reverse_iterator=typename Storage::reverse_iterator;
  using const_reverse_iterator=typename Storage::const_reverse_iterator;
  using reference=typename Storage::reference;
  using const_reference=typename Storage::const_reference;
  SEList():storage_(std::make_shared<Storage>()){}
  SEList(std::initializer_list<T> values):storage_(std::make_shared<Storage>(values)){}
  template<class I> SEList(I first,I last):storage_(std::make_shared<Storage>(first,last)){}
  explicit SEList(std::size_t count):storage_(std::make_shared<Storage>(count)){}
  SEList clone()const{return SEList(begin(),end());}
  std::size_t size()const{return storage_->size();}
  bool empty()const{return storage_->empty();}
  iterator begin(){return storage_->begin();}
  iterator end(){return storage_->end();}
  const_iterator begin()const{return storage_->begin();}
  const_iterator end()const{return storage_->end();}
  reverse_iterator rbegin(){return storage_->rbegin();}
  reverse_iterator rend(){return storage_->rend();}
  const_reverse_iterator rbegin()const{return storage_->rbegin();}
  const_reverse_iterator rend()const{return storage_->rend();}
  T* data(){return storage_->data();}
  const T* data()const{return storage_->data();}
  reference operator[](std::size_t i){return (*storage_)[i];}
  const_reference operator[](std::size_t i)const{return (*storage_)[i];}
  reference at(std::size_t i){return storage_->at(i);}
  const_reference at(std::size_t i)const{return storage_->at(i);}
  reference front(){return storage_->front();}
  const_reference front()const{return storage_->front();}
  reference back(){return storage_->back();}
  const_reference back()const{return storage_->back();}
  void reserve(std::size_t n){storage_->reserve(n);}
  void clear(){storage_->clear();}
  void pop_back(){storage_->pop_back();}
  void push_back(const T& value){storage_->push_back(value);}
  void push_back(T&& value){storage_->push_back(std::move(value));}
  template<class... A> decltype(auto) emplace_back(A&&... args){return storage_->emplace_back(std::forward<A>(args)...);}
  iterator erase(const_iterator position){return storage_->erase(position);}
  iterator erase(const_iterator first,const_iterator last){return storage_->erase(first,last);}
  bool operator==(const SEList& other)const{return *storage_==*other.storage_;}
};

using SEBytes=SEList<std::uint8_t>;
