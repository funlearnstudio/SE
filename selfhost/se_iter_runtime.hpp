#pragma once
// Eager iterators match the production extension API's List results.
inline SEList<std::int64_t> se_iter_range(std::int64_t start,std::int64_t stop,std::int64_t step=1){
  if(step==0)throw std::runtime_error("iter.range step cannot be zero.");
  SEList<std::int64_t> out;
  for(auto i=start;step>0?i<stop:i>stop;){
    out.push_back(i);
    if((step>0&&i>std::numeric_limits<std::int64_t>::max()-step)||(step<0&&i<std::numeric_limits<std::int64_t>::min()-step))break;
    i+=step;
  }
  return out;
}
inline SEList<std::int64_t> se_iter_range(std::int64_t stop){return se_iter_range(0,stop,1);}
template<class A,class B> using SEPairElement=std::conditional_t<std::is_same_v<A,B>,A,SEAny>;
template<class A,class B> auto se_iter_zip(const SEList<A>& a,const SEList<B>& b){using E=SEPairElement<A,B>;SEList<SEList<E>> out;for(std::size_t i=0;i<std::min(a.size(),b.size());++i)out.push_back(SEList<E>{E(a[i]),E(b[i])});return out;}
template<class A,class B> auto se_iter_product(const SEList<A>& a,const SEList<B>& b){using E=SEPairElement<A,B>;SEList<SEList<E>> out;for(const auto& x:a)for(const auto& y:b)out.push_back(SEList<E>{E(x),E(y)});return out;}
template<class T> auto se_iter_enumerate(const SEList<T>& values){using E=SEPairElement<std::int64_t,T>;SEList<SEList<E>> out;for(std::size_t i=0;i<values.size();++i)out.push_back(SEList<E>{E(static_cast<std::int64_t>(i)),E(values[i])});return out;}
template<class T> auto se_iter_permutations(const SEList<T>& values,std::int64_t count){
  if(count<0||static_cast<std::uint64_t>(count)>values.size())throw std::runtime_error("iter.permutations invalid r.");
  SEList<SEList<T>> out;SEList<T> current;std::vector<bool> used(values.size());
  std::function<void()> visit=[&](){if(current.size()==static_cast<std::size_t>(count)){out.push_back(current.clone());return;}for(std::size_t i=0;i<values.size();++i)if(!used[i]){used[i]=true;current.push_back(values[i]);visit();current.pop_back();used[i]=false;}};
  visit();return out;
}
template<class T> auto se_iter_permutations(const SEList<T>& values){return se_iter_permutations(values,static_cast<std::int64_t>(values.size()));}
template<class T> auto se_iter_combinations(const SEList<T>& values,std::int64_t count){
  if(count<0||static_cast<std::uint64_t>(count)>values.size())throw std::runtime_error("iter.combinations invalid r.");
  SEList<SEList<T>> out;SEList<T> current;
  std::function<void(std::size_t)> visit=[&](std::size_t start){if(current.size()==static_cast<std::size_t>(count)){out.push_back(current.clone());return;}for(std::size_t i=start;i<values.size();++i){current.push_back(values[i]);visit(i+1);current.pop_back();}};
  visit(0);return out;
}
template<class T,class... Rest> auto se_itertools_chain(const SEList<T>& first,const Rest&... rest){SEList<T> out=first.clone();auto append=[&](const auto& values){for(const auto& item:values)out.push_back(item);};(append(rest),...);return out;}
template<class T> auto se_itertools_flatten(const SEList<SEList<T>>& values){SEList<T> out;for(const auto& row:values)for(const auto& item:row)out.push_back(item);return out;}
template<class T> auto se_itertools_flatten(const SEList<T>& values){return values.clone();}
template<class T> auto se_itertools_chunked(const SEList<T>& values,std::int64_t count){if(count<1)throw std::runtime_error("Chunk size must be positive.");SEList<SEList<T>> out;for(std::size_t i=0;i<values.size();){auto end=i+std::min(values.size()-i,static_cast<std::size_t>(count));out.push_back(SEList<T>(values.begin()+i,values.begin()+end));i=end;}return out;}
template<class T> auto se_itertools_windows(const SEList<T>& values,std::int64_t count){if(count<1)throw std::runtime_error("Window size must be positive.");SEList<SEList<T>> out;if(static_cast<std::uint64_t>(count)>values.size())return out;for(std::size_t i=0;i<=values.size()-static_cast<std::size_t>(count);++i)out.push_back(SEList<T>(values.begin()+i,values.begin()+i+count));return out;}
template<class T> auto se_itertools_take(const SEList<T>& values,std::int64_t count){if(count<0)throw std::runtime_error("Count cannot be negative.");return SEList<T>(values.begin(),values.begin()+std::min(values.size(),static_cast<std::size_t>(count)));}
template<class T> auto se_itertools_drop(const SEList<T>& values,std::int64_t count){if(count<0)throw std::runtime_error("Count cannot be negative.");return SEList<T>(values.begin()+std::min(values.size(),static_cast<std::size_t>(count)),values.end());}
template<class T> auto se_itertools_cycle(const SEList<T>& values,std::int64_t count){if(count<0)throw std::runtime_error("Repeat count cannot be negative.");SEList<T> out;for(std::int64_t i=0;i<count;++i)for(const auto& value:values)out.push_back(value);return out;}
template<class T> auto se_itertools_pairs(const SEList<T>& values){SEList<SEList<T>> out;for(std::size_t i=1;i<values.size();++i)out.push_back(SEList<T>{values[i-1],values[i]});return out;}
template<class T> auto se_itertools_unique(const SEList<T>& values){SEList<T> out;SEList<std::string> texts;for(const auto& value:values){std::ostringstream stream;se_print_value(stream,value);auto text=stream.str();if(std::find(texts.begin(),texts.end(),text)==texts.end()){texts.push_back(text);out.push_back(value);}}return out;}
