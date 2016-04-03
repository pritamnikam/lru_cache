/*
 * How to compile:
 *    g++ solution.cc -g -o solution
 */

#include <cstddef>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

namespace network {

	namespace cache {

		template<typename K, typename V> class LRUCache {
			public:
				typedef typename std::pair<K, V> KeyValuePair;
				typedef typename list<KeyValuePair>::iterator KeyValuePairListIerator;

				void Put(const K& key, const V& value) {
					if (Size() && Exists(key)) {
						auto it = cache_items_map_.find(key);
						cache_items_list_.erase(it->second);
						cache_items_map_.erase(it);
					}

					// Prepend <key, value> to the |cache_items_list_| to avoid cache eviction.
					cache_items_list_.push_front(KeyValuePair(key, value));
					cache_items_map_[key] = cache_items_list_.begin();

					// Cache eviction: remove last cache entry.
					if (Size() > max_size_)	Evict();
				}

				const V& Get(const K& key) {
					if (Size() && Exists(key)) {
						auto it = cache_items_map_.find(key);
						cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
						return it->second->second;
					}

					throw range_error("There is no such key in cache");
				}

				bool Exists(const K& key) const {
					return cache_items_map_.find(key) != cache_items_map_.end();
				}

				size_t Size() const {
					return cache_items_map_.size();
				}

				template<typename Key, typename Value> class CleanupCallback {
					public:
						virtual void CleanUp(const KeyValuePair&) = 0;
				};

				LRUCache(size_t max_size, CleanupCallback<K, V>* callback) :
					max_size_(max_size), callback_(callback) {
					}

			private:
				void Evict() {
					auto eldest = cache_items_list_.end();
					eldest--;

					if (callback_) callback_->CleanUp(*eldest);
					cache_items_map_.erase(eldest->first);
					cache_items_list_.pop_back();
				}

				list<KeyValuePair> cache_items_list_;

				// Search takes O(1)
				unordered_map<K, KeyValuePairListIerator> cache_items_map_;

				// Maximum allowed cache size.
				size_t max_size_;
				CleanupCallback<K, V>* callback_;
		};

	} // namespace cache


	typedef pair<string, int> IpAndPortNumberPair;
	typedef string IpAndPortCombinedString;

	/* TCB make use of LRU cache */
	class TCB {
		public:
			void Clear() const {
				//  Does a cleanup like socket shutdown etc...
			}

			static string Combined(const IpAndPortNumberPair& key) {
				stringstream ss;
				ss << key.first << ":" << key.second;
				return ss.str();
			}

			static vector<string> Tokenize(string str, char delimeter) {
				vector<string> token_v;
				size_t start = str.find_first_not_of(delimeter), end = start;

				while (start != string::npos) {
					// Find next occurence of delimiter
					end = str.find(delimeter, start);

					// Push back the token found into vector
					token_v.push_back(str.substr(start, end - start));

					// Skip all occurences of the delimiter to find new start
					start = str.find_first_not_of(delimeter, end);
				}

				return token_v;
			}

			static IpAndPortNumberPair Retrive(const IpAndPortCombinedString& key) {
				IpAndPortNumberPair val;
				vector<string> strs = Tokenize(key, ':');
				val.first = strs[0];
				val.second = stoi(strs[1]);
				return val;
			}

			static const TCB& Get(const IpAndPortNumberPair& key) {
				return tcb_cache_->Get(Combined(key));
			}

			static void Put(const IpAndPortNumberPair& key, const TCB& tcb) {
				tcb_cache_->Put(Combined(key), tcb);
			}

			class TCBCleanupCallback :
				public cache::LRUCache<IpAndPortCombinedString, TCB>::CleanupCallback<IpAndPortCombinedString, TCB> {
					public:
						void CleanUp(const cache::LRUCache<IpAndPortCombinedString, TCB>::KeyValuePair& pair) {
							pair.second.Clear();
						}
				};

			static TCBCleanupCallback* callback_;
			static cache::LRUCache<IpAndPortCombinedString, TCB>* tcb_cache_;

	};

	TCB::TCBCleanupCallback* TCB::callback_ = new TCB::TCBCleanupCallback;
	cache::LRUCache<IpAndPortCombinedString, TCB>* TCB::tcb_cache_ =
		new cache::LRUCache<IpAndPortCombinedString, TCB>(1, TCB::callback_);
}

int main() {
	{
		network::cache::LRUCache<int, string> cache_(1, nullptr);
		cache_.Put(10, "My String");
		cache_.Put(20, "test string");
		// ASSERT_TRUE(cache_.Exists(10));
		// ASSERT_TRUE(cache_.Exists(20));
	}

	{
		network::TCB tcb_;
		network::TCB::Put(make_pair("127.0.0.1", 80), tcb_);
		network::TCB::tcb_cache_->Put(network::TCB::Combined(make_pair("192.168.0.1", 443)), tcb_);

		// ASSERT_TRUE(cache_.Exists(10));
		// ASSERT_TRUE(cache_.Exists(20));
	}
	return 0;
}

