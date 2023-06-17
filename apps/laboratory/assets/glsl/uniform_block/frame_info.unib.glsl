layout (binding = 3) uniform _FrameInfo
{
    uint64_t DepthAttachmentHandle;
    uint64_t ColorAttachmentHandle;
    uint64_t FrameIdx;
    float SecondsSinceStart;
    float SecondsSinceLastFrame;
};
