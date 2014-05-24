#include <vector>

#include "caffe/layer.hpp"
#include "caffe/util/device.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype>
void ConcatLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  Layer<Dtype>::SetUp(bottom, top);
  concat_dim_ = this->layer_param_.concat_param().concat_dim();
  CHECK_GE(concat_dim_, 0) <<
    "concat_dim should be >= 0";
  CHECK_LE(concat_dim_, 1) <<
    "For now concat_dim <=1, it can only concat num and channels";

  // Initialize with the first blob.
  count_ = bottom[0]->count();
  num_ = bottom[0]->num();
  channels_ = bottom[0]->channels();
  height_ = bottom[0]->height();
  width_ = bottom[0]->width();
  for (int i = 1; i < bottom.size(); ++i) {
    count_ += bottom[i]->count();
    if (concat_dim_== 0) {
      num_ += bottom[i]->num();
    } else if (concat_dim_ == 1) {
      channels_ += bottom[i]->channels();
    } else if (concat_dim_ == 2) {
      height_ += bottom[i]->height();
    } else if (concat_dim_ == 3) {
      width_ += bottom[i]->width();
    }
  }
  (*top)[0]->Reshape(num_, channels_, height_, width_);
  CHECK_EQ(count_, (*top)[0]->count());
}

template <typename Dtype>
Dtype ConcatLayer<Dtype>::Forward(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  Dtype* top_data = (*top)[0]->mutable_data();
  if (concat_dim_== 0) {
    int offset_num = 0;
    for (int i = 0; i < bottom.size(); ++i) {
      const Dtype* bottom_data = bottom[i]->const_data();
      int num_elem = bottom[i]->count();
      this->device_->copy(num_elem, bottom_data,
                        top_data+(*top)[0]->offset(offset_num));
      offset_num += bottom[i]->num();
    }
  } else if (concat_dim_ == 1) {
    int offset_channel = 0;
    for (int i = 0; i < bottom.size(); ++i) {
      const Dtype* bottom_data = bottom[i]->const_data();
      int num_elem =
        bottom[i]->channels()*bottom[i]->height()*bottom[i]->width();
      for (int n = 0; n < num_; ++n) {
        this->device_->copy(num_elem, bottom_data+bottom[i]->offset(n),
          top_data+(*top)[0]->offset(n, offset_channel));
      }
      offset_channel += bottom[i]->channels();
    }
  } else {
    LOG(FATAL) << "concat_dim along dim" << concat_dim_ <<
      " not implemented yet";
  }
  return Dtype(0.);
}

template <typename Dtype>
void ConcatLayer<Dtype>::Backward(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* top_diff = top[0]->const_diff();
  if (concat_dim_ == 0) {
    int offset_num = 0;
    for (int i = 0; i < bottom->size(); ++i) {
      Blob<Dtype>* blob = (*bottom)[i];
      Dtype* bottom_diff = blob->mutable_diff();
      this->device_->copy(blob->count(),
        top_diff+top[0]->offset(offset_num), bottom_diff);
      offset_num += blob->num();
    }
  } else if (concat_dim_ == 1) {
    int offset_channel = 0;
    for (int i = 0; i < bottom->size(); ++i) {
      Blob<Dtype>* blob = (*bottom)[i];
      Dtype* bottom_diff = blob->mutable_diff();
      int num_elem = blob->channels()*blob->height()*blob->width();
      for (int n = 0; n < num_; ++n) {
        this->device_->copy(num_elem, top_diff+top[0]->offset(n, offset_channel),
          bottom_diff+blob->offset(n));
      }
      offset_channel += blob->channels();
    }
  } else {
    LOG(FATAL) << "concat_dim along dim" << concat_dim_ <<
      " not implemented yet";
  }
}

INSTANTIATE_CLASS(ConcatLayer);

}  // namespace caffe
