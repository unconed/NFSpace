/*
 *  Utility.cpp
 *  NFSpace
 *
 *  Sources:
 *    http://www.ogre3d.org/wiki/index.php/CropImage
 *    http://www.ogre3d.org/forums/viewtopic.php?f=3&t=46850
 *    http://www.ogre3d.org/forums/viewtopic.php?p=189032
 *
 *  Created by Steven Wittens on 20/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Utility.h"

namespace NFSpace {
    
/**
 * Little utility function that crops an image
 * (Doesn't alter the source image, returns a cropped representation)
 *
 * @param   source   The source image
 * @param   offsetX   The X offset from the origin
 * @param   offsetY   The Y offset from the origin
 * @param   width   The width to crop to
 * @param   height   The height to crop to
 *
 * @return   Returns the cropped representation of the source image if the parameters are valid,
 *         otherwise, returns the source image.
 */
Ogre::Image cropImage(const Ogre::Image& source, size_t offsetX, size_t offsetY, size_t width, size_t height)
{
    if(offsetX + width > source.getWidth())
        return source;
    else if(offsetY + height > source.getHeight())
        return source;
    
    size_t bpp = Ogre::PixelUtil::getNumElemBytes(source.getFormat());

    const uchar* srcData = source.getData();
    const uchar* srcPointer = srcData;

    uchar* dstData = OGRE_ALLOC_T(uchar, width * height * bpp, MEMCATEGORY_RENDERSYS);
    uchar *dstPointer = dstData;
    
    size_t srcPitch = source.getRowSpan();
    size_t dstPitch = width * bpp;
    
    srcPointer += offsetY * srcPitch + offsetX * bpp;
    
    for (size_t row = 0; row < height; row++) {
        memcpy(dstPointer, srcPointer, dstPitch);
        srcPointer += srcPitch;
        dstPointer += dstPitch;
    }
    
    Ogre::Image croppedImage;
    croppedImage.loadDynamicImage(dstData, width, height, 1, source.getFormat(), false);
    
    return croppedImage;
}

    void saveTexture(Ogre::TexturePtr texture) {
        static int out = 0;
        String filename = "out-" + StringConverter::toString(++out) + ".png";
        Ogre::PixelFormat pf = PF_BYTE_RGBA; // texture->getFormat()
        
        // Declare buffer
        const size_t bufferSize = texture->getWidth() * texture->getHeight() * Ogre::PixelUtil::getNumElemBytes(pf);
        unsigned char *data = OGRE_ALLOC_T(uchar, bufferSize, Ogre::MEMCATEGORY_GENERAL);
        
        // Clear buffer
        memset(data, 0, bufferSize);
        
        // Setup Image with correct settings
        Ogre::Image image;
        image.loadDynamicImage(data, texture->getWidth(), texture->getHeight(), 1, pf, true);
        
        // Copy Texture buffer contents to image buffer
        Ogre::HardwarePixelBufferSharedPtr buffer = texture->getBuffer();      
        const Ogre::PixelBox destBox = image.getPixelBox();
        buffer->blitToMemory(destBox);
        
        // Save to disk!
        image.save(filename);
    }
    
    void updateSceneManagersAfterMaterialsChange()
    {
        if (Ogre::Pass::getDirtyHashList().size() != 0 || Ogre::Pass::getPassGraveyard().size() != 0) {
            Ogre::SceneManagerEnumerator::SceneManagerIterator scenesIter = Root::getSingleton().getSceneManagerIterator();
            
            while(scenesIter.hasMoreElements()) {
                Ogre::SceneManager* pScene = scenesIter.getNext();
                
                if (pScene) {
                    Ogre::RenderQueue* pQueue = pScene->getRenderQueue();
                    
                    if (pQueue) {
                        Ogre::RenderQueue::QueueGroupIterator groupIter = pQueue->_getQueueGroupIterator();
                        while (groupIter.hasMoreElements()) {
                            Ogre::RenderQueueGroup* pGroup = groupIter.getNext();
                            if(pGroup)
                                pGroup->clear(false);
                        }
                    }
                }
            }
            
            // Now trigger the pending pass updates
            Ogre::Pass::processPendingPassUpdates();
        }
    }
};