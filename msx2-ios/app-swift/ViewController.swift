/**
 * micro MSX2+ - Example for iOS Swift
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
import UIKit
import MSX2

class ViewController: UIViewController, MenuViewDelegate {
    private let menuView = MenuView()
    private let msx2View = MSX2View()

    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = .black
        menuView.delegate = self
        msx2View.backgroundColor = .blue
        view.addSubview(menuView)
        view.addSubview(msx2View)
        DispatchQueue.global(qos: .default).async {
            let main = NSData(contentsOfFile: Bundle.main.path(forResource: "cbios_main_msx2+_jp", ofType: "rom")!)!
            let logo = NSData(contentsOfFile: Bundle.main.path(forResource: "cbios_logo_msx2+", ofType: "rom")!)!
            let sub = NSData(contentsOfFile: Bundle.main.path(forResource: "cbios_sub", ofType: "rom")!)!
            let rom = NSData(contentsOfFile: Bundle.main.path(forResource: "game", ofType: "rom")!)!
            self.msx2View.setup(withCBiosMain: main as Data,
                                logo: logo as Data,
                                sub: sub as Data,
                                rom: rom as Data,
                                romType: .normal,
                                select: 0x1B,
                                start: 0x20)
        }
    }

    override func viewDidAppear(_ animated: Bool) {
        resize()
    }

    func menuViewDidPushResetButton() {
        msx2View.reset()
    }

    func resize() {
        let x = view.safeAreaInsets.left
        var y = view.safeAreaInsets.top
        let width = view.frame.size.width - x - view.safeAreaInsets.right
        //let height = view.frame.size.height - y - view.safeAreaInsets.top

        // layout MenuView
        menuView.frame = CGRectMake(x, y, width, 44)
        y += 44

        // layout MSX2View
        let h = width * 480.0 / 568.0
        msx2View.frame = CGRectMake(x, y, width, h)
        //y += h

        // TODO: layout VirtualPadView
    }
}

