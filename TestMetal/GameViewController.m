//
//  GameViewController.m
//  TestMetal
//
//  Created by Sigurd Seteklev on 14/06/2024.
//

#import "GameViewController.h"
#import "Renderer.h"

@implementation GameViewController
{
    MTKView *_view;

    Renderer *_renderer;
    
    // Engine state
    EngineStateStruct *_engineState;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.drawableSize];

    _view.delegate = _renderer;
    
    // Initialize engine
    _engineState = initialize();
    if (_engineState) {
        _engineState->state = ENGINE_STATE_RUNNING;
        NSLog(@"✅ Engine initialized successfully");
        
        // Pass engine state to renderer
        [_renderer setEngineState:_engineState];
    } else {
        NSLog(@"❌ Failed to initialize engine");
    }
}

- (void)dealloc {
    // Shutdown engine
    if (_engineState) {
        engine_shutdown(_engineState);
        _engineState = NULL;
        NSLog(@"✅ Engine shutdown successfully");
    }
}

@end
