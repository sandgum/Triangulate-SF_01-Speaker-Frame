# Devlog 1: Started the Design

Preface: I already have 4 additional hours logged on the Lookout app, but have no way of having Hackatime recognise those 4 hours. That is where I started the design and completed most of the things I detail in this devlog. Here is the proof:

<img width="3024" height="4032" alt="IMG_1385" src="https://github.com/user-attachments/assets/7beb6ee7-f463-4062-b33d-d67d323ca7d8" />

Now, onto the real devlog.

Soooooo, my parents are crazy about getting a digital picture frame, and after resarching, they were able to find... either hundreds of cheap RK3326-based Android picture frames (which aren't even that big), or amazing speakers from IKEA or Samsung *disguised* as frames, but not actually having screens.
I actually really like the idea of a digital picture frame with a speaker, and an ACTUALLY GOOD speaker at that (so it's not just a TV). So I started researching, using my knowledge in audio and electronics systems to try and find optimal components for what I am trying to do.

Then, I started the design of the frame which will enclose everything, including the all-important monitor and speaker system. Speaking of the speaker system (hehe), I decided to use some parts which I already had laying around, including at least (yes, at least) two Peerless P830985 drivers as bass-mid woofers, and using four Dayton Audio ND16FA-6 tweeters for the highs. This system should offer at least 100W of total RMS power output, around 60W of which will be coming from the bass-mid woofers.

For the main display, I wanted to use a large size, like 24 or 27 inches. This should make the viewing experience nice, and also make part sourcing very easy with many standard monitors being exactly those sizes. Speaking of monitors, I was able to find a really nice LG 24 inch HD monitor with an IPS display and 120Hz refresh rate for around $100AUD. This is MUCH cheaper than equivalent barebones displays and driver boards I could find on Alibaba, plus it offers VESA mounts which i can easily bolt my enclosure to.

Now to finally start the design of the frame. Audio around a screen is always a challenge, especially when you’re trying to keep it high-fidelity and actually worth listening to. This is because the screen makes up your entire frontal facade, leaving almost no room for high-frequency drivers. Designing a frame, I do have some leeway as to bezel size around the screen, but I don’t want the speaker system to be overly noticeable, while still providing omnidirectional audio (It’s gonna be in a living room, after all).

That led me to place the tweeters at the corners of the screen, in a 45-ish degree angle in two axes, so they face outwards from the screen. This decision will come back to bite me later. After that, I designed a nice profile for the frame to follow while keeping space for the tweeters inside yet not making their presence obvious. I was successful in making this, deciding to go with a very rounded frame design with rounded corners all around.

That’s when I thought to myself, “Frames aren’t really rounded rectangles, they’re usually sharp”, which is a correct thought. My frame did not look like a photo frame at all, instead looking like a big monitor with giant bezels. I hurried to try and make some corners for the frame, which turned out very successful. I then decided to make those corners magnetically attached, allowing the user to pick their style.

Next, I started to solve the problem of speaker holes for the tweeters. I had one goal in mind, that these speaker holes should not look like speaker holes at all, which means NO circles, like I usually do. That led me to experiment with some different styles for speaker holes, such as a “rainbow” made from rounded slots. That looked terrible, and completely blocked the tweeters from (from their perspective) above. Choosing to angle the tweeters introduces severe complexities, as the speaker holes now needed to simultaneously face both upwards and forward while looking good.

My “rainbow” design did have some good things about it, as it led me to the final design using splines that gradually open up to the speaker holes before gracefully closing down to a point again. I was also able to make a 45 degree tweeter-facing hole which allows it to breathe without looking like it faces 45 degrees. the corner pieces were also cut to make room for air, and they ended up looking even better after that.

Finally, I had to make the actual housing for the tweeters, as currently they were floating in thin air. Making this look good was quite a task, utilising many face offsets to close microscopic gaps and clean up the model. Eventually, I added some speaker holes at the back of the frame for the bass-mid woofers, and that’s all!

<img width="944" height="900" alt="image" src="https://github.com/user-attachments/assets/bd9c46cd-fec3-4ea5-8785-81a8695c8ec3" />
<img width="1386" height="900" alt="image" src="https://github.com/user-attachments/assets/042e0ca0-734e-4231-bf40-03d22e4d340b" />
<img width="400" height="900" alt="image" src="https://github.com/user-attachments/assets/1bf897cc-50af-4e6b-82a9-349adab15214" />

# Devlog 2: Designed the Subwoofer Box

Immediately upon finishing the design of the main frame, I started grinding out the subwoofer box that would go behind the monitor. As I had a huge amount of space behind the frame, I thought that maybe I should add... I don't know... FOUR extra bass-mid woofers to the design??? Since each of the drivers consumes around 30W, that would result in more than 180W of RMS power output... JUST FROM THE BASS DRIVERS. To put that into perspective, the 6 inch subwoofer used in my home theatre setup is 150W... so this should make for a ridiculous sound system.

To make the box itself, I used 5mm thick walls all around the box, and 10mm thick walls to hold the drivers themselves in. That should provide more than enough structural bracing for the 3D-printed box to hold itself together when the full 180W of driver power is unleashed within it. To make the box even stronger, I hollowed out a section right in the middle of the box for VESA mounts and the basic electronics of the whole system.

I'm super proud of this design feature in particular, which allows me to assemble the whole picture frame and secure all load-bearing components to it using the standard 100x100mm VESA mount the monitor comes with. That same VESA mount will also be used for securing the stand or the wall-mount accessory to the frame, so it's a really smart design feature.

Anyways, there's not that much more I can talk about regarding a literal box, so that's all for this devlog!

<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/8cace9af-9fe8-4a95-8d67-dc2311c542f3" />
<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/ac89b385-21d2-4f38-a47c-0364b34dc0c1" />
<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/9da84a2b-176c-4cd0-972c-de71955f024a" />

# Devlog 3: Splitting the Frame, and Ingenious Assembly!

I want the SF_01 to be 3D-printed without incurring too many costs. The main driver for cost in 3D-Printing is the size of the part, as bigger parts require bigger and exponentially expensive and inaccessible printers to print. That's why I want the SF_01 to be entirely printable in a standard 256x256x256mm build volume used by Bambu Lab.

Since the frame is 24 inches diagonally, it results in the largest dimension of the frame being over half a metre long, which requires it to be split into a minimum of 6 parts. For the main frame, those 6 parts are just the four corners of the frame plus two straight middle sections. Splitting the frame into parts is also what crucially allows for the monitor to slide into it in the first place, with it being held securely after that.

That's why I designed the main frame to be assembled into two main parts: a top and bottom part. The three top and bottom parts will be permanently glued together, but the two halves can join up and be removed at any time. How will I accomplish the two main parts being securely held together without compromising the aesthetics of the frame via unsightly screws, all while making sure the frame is repairable?

That's where the subwoofer box comes into play. I designed the box to hold 6 drivers, all of which face towards the outside of the frame. Each speaker requires four mounting screws to secure it into place, which I used to my advantage. I intentionally used cap-head screws, which stick out quite a distance from the drivers when screwed in. Those cap head screws then slide into grooves in the main frame from the back, holding the two pieces together! Finally, the subwoofer box is secured from the back with VESA screws, and the whole thing is securely assembled, with only those 4 VESA screws holding the entire frame together!

# Devlog 4: Splitting the Subwoofer Box

The subwoofer box itself is also way too large to fit on the build plate of any reasonable printer, so I set out to split that into multiple parts as well. However, I needed to be considerate with how I slice this part, as it has to deal with immense structural loads and still be completely airtight, as it's got 180W of speaker power blasting through it. That's why I decided to split the subwoofer box in 7 pieces instead of 6, with the baffle areas that hold the drivers all being crucially printed in one piece, and also the all-important VESA mounting area is in one piece as well. These are the two areas which will bear the most load, and cutting the box in this way ensures that there are no seams running through those crucial parts.

As for how the box is going to be assembled, I don't intend for the box to be able to be disassembled in the future, and also the box needs to provide an airtight seal. So I opted to omit any clips or dowels, and left the borders between the box's pieces blank for superglue to flow in between the pieces. It doesn't affect the repairability of the frame at all, as all the components in the box can still be removed freely when the whole thing is glued together.

# Devlog 5: Mounting Options!

The SF_01 is a FRAME, so it makes sense that it would be mounted either on someone's wall or on a table of some sorts. Both mounting options need to be addressed, and both must be robust enough to hold the gigantic 24 inch screen and especially the heavy drivers securely. So I designed the wall-mounting panel to screw into the monitor's VESA mounting holes, which are literally designed to hold up the full weight of the monitor. I then designed the wall-mounting panel, and made it absolutely humongous to be able to distribute the stuctural load of the frame's weight over a large area. I also made the screw keyholes extra thick and able to accommodate large screws that can be driven deep into the user's wall. I then also added an option to orient the mounting holes so that the frame is in portrait mode.

Then, I focused on designing the stand, which will bolt onto the VESA mount as a separate piece to the wall-mount piece. That decision was made because the stand, when folded, is not perfectly flush to the back of the frame, which would interfere with wall-mounting. I made the stand extremely chunky, with it hinging diagonal to the frame. That allows for the stand to work in both orientations, and also makes it more stable. Finally, I added a built-in stopper which prevents the stand from hinging out more than it should.

# Devlog 6: Started PCB Schematic

I finally finished the enclosure for the SF_01, and started on the schematic for the PCB. For Bluetooth, I decided to use an ESP32-S3 WROOM 2 module for audio streaming and DSP calculations. This was because the ESP32 is a super familiar platform for me, and literally all other Bluetooth DSP chips are from Qualcomm, who are total party poopers and make SURE you can't access support or any SDKs for any of their chips, making them impossible to use for a random kid like me.

Next, I added the Raspberry Pi CM4, and started wiring up HDMI for it to push an image out to the monitor. I used this as its cheap (MUCH cheaper than the CM5), and it only needs to run ONE web app on like Android 10, so 1GB of RAM and the CM4 performance is plenty good enough. Plus, the CM4 is just so damn EASY to make anything for (Take some notes Qualcomm), and it comes with an easy way to use WiFi (it comes built-in!) and to route a USB port. I made quick work of that schematic, especially since I was using the bare minimum of the CM4's pins. Just HDMI and one USB port with manual Host/Device switching (The CM4 only needs to be in Device mode once, during flashing Android, and it can stay in Host mode forever after that for Keyboard/Mouse support).

# Devlog 7: AUDIO NIGHTMARES

Having promptly dealt with the digital side of the PCB, I turned my gaze towards the analog section of the board that carries audio signals and the amplifiers. Yes, amplifierS, plural. See, I wanted the SF_01 to have something special. Since it has so many drivers, and is so physically huge, it should be able to reproduce stereo audio quite nicely, especially with the angled tweeters. Since the frame could be in two orientations, however, I thought that maybe I could dynamically switch which drivers the stereo signal plays out of. This would require audio switches, whic reroute the signals generated by my PCM5102A DACs (One set of low frequency left/right for the woofers, one set of high frequency left/right for the tweeters).

So I started to put two of the switches together, outputting audio to pre-connected sets of tweeters and woofers. That was a mistake, as I had failed to realise that electrically-connected drivers are... ELECTRICALLY CONNECTED. PERNANENTLY. How I had wired the switches literally sent out the same audio signal to every driver regardless of orientation. I then realised that the proper implementation of this is to have SEVERAL switches in series, which all work together to route signals.

My schematic went from relatively simple to having SEVEN audio switches and being too complicated to explain. But trust me, it definitely works. I made sure with a truth table. Now, I had to face the biggest beast of them all, the amplifiers. My schematic called for FIVE individual stereo amplifier chips, dedicating a whole channel for each driver. Soooo... I hope I survive...
