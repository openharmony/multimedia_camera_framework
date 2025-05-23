/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_CAMERA_DPS_WRITER_H
#define OHOS_CAMERA_DPS_WRITER_H

#include "muxer.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
using namespace MediaAVCodec;
class Writer {
public:
    Writer() = default;
    virtual ~Writer();

    MediaManagerError Create(int32_t outputFd,
        const std::map<Media::Plugins::MediaType, std::shared_ptr<Track>>& trackMap);
    MediaManagerError Write(Media::Plugins::MediaType type, const std::shared_ptr<AVBuffer>& sample);
    MediaManagerError Start();
    MediaManagerError Stop();
    MediaManagerError AddMediaInfo(const std::shared_ptr<MediaInfo>& mediaInfo);
    MediaManagerError AddUserMeta(const std::shared_ptr<Meta>& userMeta);

    inline void SetLastPause(int64_t lastPause)
    {
        lastPause_ = lastPause;
    }

private:
    MediaManagerError CreateTracksAndMuxer();

    std::shared_ptr<Muxer> outputMuxer_ {nullptr};
    int64_t lastPause_ {-1};
    int32_t outputFileFd_ {-1};
    bool started_ {false};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_WRITER_H