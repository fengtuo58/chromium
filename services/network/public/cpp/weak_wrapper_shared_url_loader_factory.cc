// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

#include "services/network/public/cpp/wrapper_shared_url_loader_factory.h"

namespace network {

WeakWrapperSharedURLLoaderFactory::WeakWrapperSharedURLLoaderFactory(
    mojom::URLLoaderFactory* factory_ptr)
    : factory_ptr_(factory_ptr) {}

WeakWrapperSharedURLLoaderFactory::WeakWrapperSharedURLLoaderFactory(
    base::OnceCallback<mojom::URLLoaderFactory*()> make_factory_ptr)
    : make_factory_ptr_(std::move(make_factory_ptr)), factory_ptr_(nullptr) {}

void WeakWrapperSharedURLLoaderFactory::Detach() {
  factory_ptr_ = nullptr;
  make_factory_ptr_.Reset();
}

void WeakWrapperSharedURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<mojom::URLLoader> loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (!factory())
    return;
  factory()->CreateLoaderAndStart(std::move(loader), routing_id, request_id,
                                  options, request, std::move(client),
                                  traffic_annotation);
}

void WeakWrapperSharedURLLoaderFactory::Clone(
    mojo::PendingReceiver<mojom::URLLoaderFactory> receiver) {
  if (!factory())
    return;
  factory()->Clone(std::move(receiver));
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
WeakWrapperSharedURLLoaderFactory::Clone() {
  mojo::PendingRemote<mojom::URLLoaderFactory> factory_remote;
  if (factory())
    factory()->Clone(factory_remote.InitWithNewPipeAndPassReceiver());
  return std::make_unique<WrapperSharedURLLoaderFactoryInfo>(
      std::move(factory_remote));
}

WeakWrapperSharedURLLoaderFactory::~WeakWrapperSharedURLLoaderFactory() =
    default;

mojom::URLLoaderFactory* WeakWrapperSharedURLLoaderFactory::factory() {
  if (make_factory_ptr_) {
    DCHECK(!factory_ptr_);
    factory_ptr_ = std::move(make_factory_ptr_).Run();
  }
  return factory_ptr_;
}

}  // namespace network
