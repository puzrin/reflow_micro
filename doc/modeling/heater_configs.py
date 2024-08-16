from hotplate_model import HotplateModel

heaters = []

heaters.append(HotplateModel()
    .label("80x70x3.8, 1.6R")
    .calibrate(T=25, R=1.6)
    .calibrate(T=102, W=11.63, V=5)
    .calibrate(T=146, W=20.17, V=7) # 151?
    .calibrate(T=193, W=29.85, V=9)
    .calibrate(T=220, W=40.66, V=11)
    .calibrate(T=255, W=52.06, V=13)
    .calibrate(T=286, W=64.22, V=15)
    .calibrate(T=310, W=77.55, V=17))
