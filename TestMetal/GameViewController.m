//
//  GameViewController.m
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

#import "GameViewController.h"
#import "engine_main.h"

@implementation GameViewController
{
    MTKView *_view;
    
    // Engine state - now only accessed through engine_main interface
    EngineStateStruct *_engineState;
}

- (void)viewDidLoad
{
    NSLog(@"viewDidLoad");
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    // Set view delegate to handle rendering first
    _view.delegate = self;
    
    // Initialize engine with Metal using the new interface
    // Use default viewport size if drawable size is not yet available
    float width = _view.drawableSize.width > 0 ? _view.drawableSize.width : 800.0f;
    float height = _view.drawableSize.height > 0 ? _view.drawableSize.height : 600.0f;
    
    // Get the bundle resource path
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    
    _engineState = engine_initialize((__bridge MetalViewHandle)_view, width, height, [resourcePath UTF8String]);
    
    if (!_engineState) {
        NSLog(@"❌ Failed to initialize engine with Metal");
        return;
    }
    
    NSLog(@"✅ Engine initialized successfully with Metal");
}

- (void)dealloc {
    // Shutdown engine using engine_main interface
    if (_engineState) {
        engine_shutdown(_engineState);
        _engineState = NULL;
        NSLog(@"✅ Engine shutdown successfully");
    }
}

#pragma mark - MTKViewDelegate

- (void)drawInMTKView:(nonnull MTKView *)view {
    // Update engine (which includes rendering) using engine_main interface
    engine_update(_engineState);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"📏 drawableSizeWillChange called: %.0f x %.0f", size.width, size.height);
    // Update viewport size using engine_main interface
    engine_resize_viewport(_engineState, size.width, size.height);
}

@end
