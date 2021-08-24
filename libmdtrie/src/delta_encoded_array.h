#ifndef BITMAP_DELTA_ENCODED_ARRAY_H_
#define BITMAP_DELTA_ENCODED_ARRAY_H_

#include <vector>

#include "bitmap.h"
#include "bitmap_array.h"
#include "utils.h"

#define USE_PREFIXSUM_TABLE 1

namespace bitmap {

template<typename T, uint32_t sampling_rate = 128>
class DeltaEncodedArray {
 public:
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef uint8_t width_type;


  DeltaEncodedArray() {
    samples_ = NULL;
    delta_offsets_ = NULL;
    deltas_ = NULL;
  }

  virtual ~DeltaEncodedArray() {
    if (samples_) {
      delete samples_;
      samples_ = NULL;
    }
    if (delta_offsets_) {
      delete delta_offsets_;
      delta_offsets_ = NULL;
    }
    if (deltas_) {
      delete deltas_;
      deltas_ = NULL;
    }
  }

  // Serialization and De-serialization
  virtual size_type Serialize(std::ostream& out) {
    size_type out_size = 0;

    out_size += samples_->Serialize(out);
    out_size += delta_offsets_->Serialize(out);
    out_size += deltas_->Serialize(out);

    return out_size;
  }

  virtual size_type Deserialize(std::istream& in) {
    size_type in_size = 0;

    samples_ = new UnsizedBitmapArray<T>();
    in_size += samples_->Deserialize(in);
    delta_offsets_ = new UnsizedBitmapArray<pos_type>();
    in_size += delta_offsets_->Deserialize(in);
    deltas_ = new Bitmap();
    in_size += deltas_->Deserialize(in);

    return in_size;
  }


 protected:
  // Get the encoding size for an delta value
  virtual width_type EncodingSize(T delta) = 0;

  // Encode the delta values
  virtual void EncodeDeltas(std::vector<T> & deltas, size_type num_deltas) = 0;

  virtual void EncodeLastDelta(T delta, pos_type pos) = 0;

  // Encode the delta encoded array
  void Encode(std::vector<T> & elements, size_type num_elements) {
    if (num_elements == 0) {
      return;
    }
    num_elements_ = num_elements;

#ifdef DEBUG
    assert(std::is_sorted(elements, elements + num_elements));
#endif

    // T max_sample = 0;
    std::vector<T> samples, deltas;
    std::vector<pos_type> delta_offsets;
    T last_val = 0;
    // uint64_t tot_delta_count = 0, delta_count = 0;
    // uint64_t delta_enc_size = 0;
    size_type cum_delta_size = 0;
    // pos_type max_offset = 0;
    width_type sample_bits, delta_offset_bits;

    for (size_t i = 0; i < num_elements; i++) {
      if (i % sampling_rate == 0) {
        samples.push_back(elements[i]);
        // if (elements[i] > max_sample) {
        //   max_sample = elements[i];
        // // }
        // if (cum_delta_size > max_offset)
        //   max_offset = cum_delta_size;
        delta_offsets.push_back(cum_delta_size);
        // num_samples_++;
        // if (i != 0) {
        //   assert(delta_count == sampling_rate - 1);
        // //   tot_delta_count += delta_count;
        // //   delta_count = 0;
        // }
      } else {
        // assert(elements[i] > last_val);
        T delta = elements[i] - last_val;
        deltas.push_back(delta);

        // delta_enc_size = EncodingSize(delta);
        cum_delta_size += EncodingSize(delta);
        // delta_count++;
      }
      last_val = elements[i];
    }
    // tot_delta_count += delta_count;

    // assert(tot_delta_count == deltas.size());
    // assert(samples.size() + deltas.size() == num_elements);
    // assert(delta_offsets.size() == samples.size());

    // sample_bits = Utils::BitWidth(max_sample);
    sample_bits = 32;
    // delta_offset_bits = Utils::BitWidth(max_offset);
    delta_offset_bits = 32;

    if (samples.size() == 0) {
      samples_ = NULL;
    } else {
      samples_ = new UnsizedBitmapArray<T>(&samples[0], samples.size(),
                                           sample_bits);
    }

    last_val_ = last_val;
    // std::cerr << last_val_ << std::endl;

    if (cum_delta_size == 0) {
      deltas_ = NULL;
    } else {
      deltas_ = new Bitmap(cum_delta_size);
      EncodeDeltas(deltas, deltas.size());
    // EncodeDeltas(&deltas[0], deltas.size());
    }

    if (delta_offsets.size() == 0) {
      delta_offsets_ = NULL;
    } else {
      delta_offsets_ = new UnsizedBitmapArray<pos_type>(&delta_offsets[0],
                                                        delta_offsets.size(),
                                                        delta_offset_bits);
    }
  }

  UnsizedBitmapArray<T>* samples_;
  UnsizedBitmapArray<pos_type>* delta_offsets_;
  Bitmap* deltas_;
//   New variable stored
  size_type num_elements_ = 0;
  // size_type num_samples_ = 0;
  T last_val_;

  private:
};

static struct EliasGammaPrefixSum {
 public:
  typedef uint16_t block_type;

  EliasGammaPrefixSum() {
    for (uint64_t i = 0; i < 65536; i++) {
      uint16_t val = (uint16_t) i;
      uint64_t count = 0, offset = 0, sum = 0;
      while (val && offset <= 16) {
        int N = 0;
        while (!GETBIT(val, offset)) {
          N++;
          offset++;
        }
        offset++;
        if (offset + N <= 16) {
          sum += ((val >> offset) & ((uint16_t) low_bits_set[N])) + (1 << N);
          offset += N;
          count++;
        } else {
          offset -= (N + 1);
          break;
        }
      }
      prefixsum_[i] = (offset << 24) | (count << 16) | sum;
    }
  }

  uint8_t offset(const block_type i) const {
    return ((prefixsum_[(i)] >> 24) & 0xFF);
  }

  uint8_t count(const block_type i) const {
    return ((prefixsum_[i] >> 16) & 0xFF);
  }

  uint16_t sum(const block_type i) const {
    return (prefixsum_[i] & 0xFFFF);
  }

 private:
  uint32_t prefixsum_[65536];
} elias_gamma_prefix_table;

template<typename T, uint32_t sampling_rate = 128>
class EliasGammaDeltaEncodedArray : public DeltaEncodedArray<T, sampling_rate> {
 public:
  typedef typename DeltaEncodedArray<T>::size_type size_type;
  typedef typename DeltaEncodedArray<T>::pos_type pos_type;
  typedef typename DeltaEncodedArray<T>::width_type width_type;

  using DeltaEncodedArray<T>::EncodingSize;
  using DeltaEncodedArray<T>::EncodeDeltas;
  using DeltaEncodedArray<T>::EncodeLastDelta;

  EliasGammaDeltaEncodedArray()
      : DeltaEncodedArray<T>() {
  }

  EliasGammaDeltaEncodedArray(std::vector<T> & elements, size_type num_elements)
      : EliasGammaDeltaEncodedArray<T>() {
    this->Encode(elements, num_elements);
  }

  virtual ~EliasGammaDeltaEncodedArray() {
  }

  T Get(pos_type i) {
    // Get offsets

    // if (i == 21){
    //   raise(SIGINT);
    // }
    
    pos_type samples_idx = i / sampling_rate;
    pos_type delta_offsets_idx = i % sampling_rate;
    T val = this->samples_->Get(samples_idx);

    if (delta_offsets_idx == 0)
      return val;

    pos_type delta_offset = this->delta_offsets_->Get(samples_idx);
    val += PrefixSum(delta_offset, delta_offsets_idx);
    return val;
  }

  T operator[](pos_type i) {
    return Get(i);
  }

  uint64_t size_overhead(){
    uint64_t size = 0;

  // UnsizedBitmapArray<T>* samples_;
  // UnsizedBitmapArray<pos_type>* delta_offsets_;
  // Bitmap* deltas_;
  // size_type num_elements_ = 0;
  // size_type num_samples_ = 0;
  // T last_val_;

    size += this->samples_->GetSize() + sizeof(width_type);
    size += this->delta_offsets_->GetSize() + sizeof(width_type);
    size += this->deltas_->GetSize();
    size += 2 * sizeof(size_type);
    size += sizeof(T);

    // Assume two pointers can be merged into one
    size += 2 * sizeof(Bitmap*);
    return size;

  }

  void Push(T element){

      width_type sample_bits = 32;
      width_type delta_offset_bits = 32;

      // if ((this->num_elements_ - 1) / sampling_rate + 1 != this->num_samples_){
      //   raise(SIGINT);
      // }
      
      // if (element == 210){
      //   raise(SIGINT);
      // }

      if (this->num_elements_ % sampling_rate == 0){
        
          size_type cum_delta_size = this->deltas_->GetSizeInBits();

          // TODO: comment these out
          // if (!this->samples_){
          //   raise(SIGINT);
          //   this->samples_ = new UnsizedBitmapArray<T>(&element, 1, sample_bits);
          // }
          // else {
          this->samples_->Push(element, sample_bits);
          // }

          // if (!this->delta_offsets_){
          //   raise(SIGINT);
          //   this->delta_offsets_ = new UnsizedBitmapArray<pos_type>(&cum_delta_size, 1, delta_offset_bits);
          // }
          // else {
          this->delta_offsets_->Push(cum_delta_size, delta_offset_bits);
          // this->num_samples_++;
          // }
      }
      else {

          T delta = element - this->last_val_;
          if (!this->deltas_){
              this->deltas_ = new Bitmap(EncodingSize(delta));
              EncodeLastDelta(delta, 0);  
          }
          else{
            pos_type pos = this->deltas_->GetSizeInBits();
            this->deltas_->Realloc_increase(EncodingSize(delta));
            // if (element == 200){
            //   raise(SIGINT);
            // }
            EncodeLastDelta(delta, pos);    
          }
      }    
      this->last_val_ = element;

      // if (Get(this->num_elements_) != element){
      //   raise(SIGINT);
      // }

      this->num_elements_ ++;

      // if ((this->num_elements_ - 1) / sampling_rate + 1 != this->num_samples_){
      //   raise(SIGINT);
      // }
  }

  int64_t BinarySearchSample(int64_t val){

    UnsizedBitmapArray<T>*arr = this->samples_;
    int64_t start = 0;
    int64_t end = (this->num_elements_ - 1) / sampling_rate;
    int64_t mid, mid_val;

    while (start <= end){

      mid = (start + end) / 2;
      mid_val = arr->Get(mid);
      if (mid_val == val){
        return mid;
      }
      else if (val < mid_val){
        end = mid - 1;
      }
      else {
        start = mid + 1;
      }
    }

    return std::max(end, INT64_C(0));

  }

  bool Find(T val) {
    // pos_type sample_off = this->samples_->LowerBound(val);
    pos_type sample_off = BinarySearchSample(val);
    pos_type current_delta_offset = this->delta_offsets_->Get(sample_off);
    val -= this->samples_->Get(sample_off);

    pos_type delta_idx = 0;
    T delta_sum = 0;
    size_type delta_max = this->deltas_->GetSizeInBits();

    while (delta_sum < val && current_delta_offset < delta_max && delta_idx < sampling_rate) {
      uint16_t block = this->deltas_->GetValPos(current_delta_offset, 16);
      uint16_t block_cnt = elias_gamma_prefix_table.count(block);
      uint16_t block_sum = elias_gamma_prefix_table.sum(block);

      if (block_cnt == 0) {
        // If the prefixsum table for the block returns count == 0
        // this must mean the value spans more than 16 bits
        // read this manually
        uint8_t delta_width = 0;
        while (!this->deltas_->GetBit(current_delta_offset)) {
          delta_width++;
          current_delta_offset++;
        }
        auto decoded_value = this->deltas_->GetValPos(current_delta_offset, delta_width) + (1ULL << delta_width);
        delta_sum += decoded_value;
        delta_idx += 1;

        // Roll back
        if (delta_idx == sampling_rate) {
          delta_idx--;
          delta_sum -= decoded_value;
          break;
        }
      } else if (delta_sum + block_sum < val) {
        // If sum can be computed from the prefixsum table
        delta_sum += block_sum;
        current_delta_offset += elias_gamma_prefix_table.offset(block);
        delta_idx += block_cnt;
      } else {
        // Last few values, decode them without looking up table
        T last_decoded_value = 0;
        while (delta_sum < val && current_delta_offset < delta_max && delta_idx < sampling_rate) {
          int delta_width = 0;
          while (!this->deltas_->GetBit(current_delta_offset)) {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          last_decoded_value = this->deltas_->GetValPos(current_delta_offset, delta_width) + (1ULL << delta_width);

          delta_sum += last_decoded_value;
          current_delta_offset += delta_width;
          delta_idx += 1;
        }

        // Roll back
        if (delta_idx == sampling_rate) {
          delta_idx--;
          delta_sum -= last_decoded_value;
          break;
        }
      }
    }

    // if (found_idx) {
    //   pos_type res = sample_off * sampling_rate + delta_idx;
    //   *found_idx = (delta_sum <= val) ? res : res - 1;
    // }
    return val == delta_sum;
  }


  bool BinarySearch(T search_val){

    pos_type sample_index = BinarySearchSample(search_val); // Works
    pos_type delta_offset = this->delta_offsets_->Get(sample_index);
    T current_val =  this->samples_->Get(sample_index);

    if (current_val == search_val){
      return true;
    }

    pos_type limit = sampling_rate;

    if (this->num_elements_ - sample_index * sampling_rate < sampling_rate){
      limit = this->num_elements_ - sample_index * sampling_rate;
    }

    T prefix_sum = 0;
    // T prefix_sum = PrefixSum(delta_offset, 0);
    // if (prefix_sum + current_val > search_val){
    //   return false;
    // }
    // if (prefix_sum + current_val == search_val){
    //   return true;
    // }    
    
    for (pos_type delta_offsets_idx = 1; delta_offsets_idx < limit; delta_offsets_idx++)
    {

      // TODO: cumulative prefixSum
      prefix_sum += PrefixSum_cumulative(delta_offset, delta_offsets_idx, delta_offsets_idx - 1);
      
      if (prefix_sum + current_val > search_val){
        return false;
      }
      if (prefix_sum + current_val == search_val){
        return true;
      }
    }

    return false;
  }

  T PrefixSum_cumulative(pos_type delta_offset, pos_type until_idx, pos_type delta_idx) {
    T delta_sum = 0;
    // pos_type delta_idx = 0;
    pos_type current_delta_offset = delta_offset;
    while (delta_idx != until_idx) {
      uint16_t block = this->deltas_->GetValPos(current_delta_offset, 16);
      uint16_t cnt = elias_gamma_prefix_table.count(block);
      if (cnt == 0) {
        // If the prefixsum table for the block returns count == 0
        // this must mean the value spans more than 16 bits
        // read this manually
        width_type delta_width = 0;
        while (!this->deltas_->GetBit(current_delta_offset)) {
          delta_width++;
          current_delta_offset++;
        }
        current_delta_offset++;
        delta_sum += this->deltas_->GetValPos(current_delta_offset, delta_width)
            + (1ULL << delta_width);
        current_delta_offset += delta_width;
        delta_idx += 1;
      } else if (delta_idx + cnt <= until_idx) {
        // If sum can be computed from the prefixsum table
        delta_sum += elias_gamma_prefix_table.sum(block);
        current_delta_offset += elias_gamma_prefix_table.offset(block);
        delta_idx += cnt;
      } else {
        // Last few values, decode them without looking up table
        while (delta_idx != until_idx) {
          width_type delta_width = 0;
          while (!this->deltas_->GetBit(current_delta_offset)) {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          delta_sum += this->deltas_->GetValPos(current_delta_offset,
                                                delta_width)
              + (1ULL << delta_width);
          current_delta_offset += delta_width;
          delta_idx += 1;
        }
      }
    }
    return delta_sum;
  }

 private:
  virtual width_type EncodingSize(T delta) override {
    return 2 * (Utils::BitWidth(delta) - 1) + 1;
  }

  virtual void EncodeDeltas(std::vector<T> & deltas, size_type num_deltas) override {
    uint64_t pos = 0;
    // raise(SIGINT);
    for (size_t i = 0; i < num_deltas; i++) {
      uint64_t delta_bits = Utils::BitWidth(deltas[i]) - 1;
      pos += delta_bits;
      assert((1ULL << delta_bits) <= deltas[i]);
      this->deltas_->SetBit(pos++);
      this->deltas_->SetValPos(pos, deltas[i] - (1ULL << delta_bits),
                               delta_bits);
      pos += delta_bits;
    }
  }

  virtual void EncodeLastDelta(T delta, pos_type pos) override{

      // if (pos == 140){
      //   raise(SIGINT);
      // }
      uint64_t delta_bits = Utils::BitWidth(delta) - 1;
    //   uint64_t pos = this->deltas_->GetSizeInBits();
      pos += delta_bits;
      this->deltas_->SetBit(pos++);
      this->deltas_->SetValPos(pos, delta - (1ULL << delta_bits),
                               delta_bits);

      if (this->deltas_->GetSizeInBits() != pos + delta_bits){
        raise(SIGINT);
      }            
  }


  T PrefixSum(pos_type delta_offset, pos_type until_idx) {
    T delta_sum = 0;
    pos_type delta_idx = 0;
    pos_type current_delta_offset = delta_offset;
    while (delta_idx != until_idx) {
      uint16_t block = this->deltas_->GetValPos(current_delta_offset, 16);
      uint16_t cnt = elias_gamma_prefix_table.count(block);
      if (cnt == 0) {
        // If the prefixsum table for the block returns count == 0
        // this must mean the value spans more than 16 bits
        // read this manually
        width_type delta_width = 0;
        while (!this->deltas_->GetBit(current_delta_offset)) {
          delta_width++;
          current_delta_offset++;
        }
        current_delta_offset++;
        delta_sum += this->deltas_->GetValPos(current_delta_offset, delta_width)
            + (1ULL << delta_width);
        current_delta_offset += delta_width;
        delta_idx += 1;
      } else if (delta_idx + cnt <= until_idx) {
        // If sum can be computed from the prefixsum table
        delta_sum += elias_gamma_prefix_table.sum(block);
        current_delta_offset += elias_gamma_prefix_table.offset(block);
        delta_idx += cnt;
      } else {
        // Last few values, decode them without looking up table
        while (delta_idx != until_idx) {
          width_type delta_width = 0;
          while (!this->deltas_->GetBit(current_delta_offset)) {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          delta_sum += this->deltas_->GetValPos(current_delta_offset,
                                                delta_width)
              + (1ULL << delta_width);
          current_delta_offset += delta_width;
          delta_idx += 1;
        }
      }
    }
    return delta_sum;
  }
};

}

#endif // BITMAP_DELTA_ENCODED_ARRAY_H_