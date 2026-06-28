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

<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/cf96a17f-be9a-4c0d-b15f-904f6dbb4a53" />
<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/d454a914-fcb0-4b3e-98d7-517eb6bb4cbc" />


# Devlog 4: Splitting the Subwoofer Box

The subwoofer box itself is also way too large to fit on the build plate of any reasonable printer, so I set out to split that into multiple parts as well. However, I needed to be considerate with how I slice this part, as it has to deal with immense structural loads and still be completely airtight, as it's got 180W of speaker power blasting through it. That's why I decided to split the subwoofer box in 7 pieces instead of 6, with the baffle areas that hold the drivers all being crucially printed in one piece, and also the all-important VESA mounting area is in one piece as well. These are the two areas which will bear the most load, and cutting the box in this way ensures that there are no seams running through those crucial parts.

As for how the box is going to be assembled, I don't intend for the box to be able to be disassembled in the future, and also the box needs to provide an airtight seal. So I opted to omit any clips or dowels, and left the borders between the box's pieces blank for superglue to flow in between the pieces. It doesn't affect the repairability of the frame at all, as all the components in the box can still be removed freely when the whole thing is glued together.

<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/1706a550-984f-4f7d-af06-311304d5879a" />
<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/fb0ef67d-1d4e-4bd8-bbbc-d514c2989b6b" />

# Devlog 5: Mounting Options!

The SF_01 is a FRAME, so it makes sense that it would be mounted either on someone's wall or on a table of some sorts. Both mounting options need to be addressed, and both must be robust enough to hold the gigantic 24 inch screen and especially the heavy drivers securely. So I designed the wall-mounting panel to screw into the monitor's VESA mounting holes, which are literally designed to hold up the full weight of the monitor. I then designed the wall-mounting panel, and made it absolutely humongous to be able to distribute the stuctural load of the frame's weight over a large area. I also made the screw keyholes extra thick and able to accommodate large screws that can be driven deep into the user's wall. I then also added an option to orient the mounting holes so that the frame is in portrait mode.

Then, I focused on designing the stand, which will bolt onto the VESA mount as a separate piece to the wall-mount piece. That decision was made because the stand, when folded, is not perfectly flush to the back of the frame, which would interfere with wall-mounting. I made the stand extremely chunky, with it hinging diagonal to the frame. That allows for the stand to work in both orientations, and also makes it more stable. Finally, I added a built-in stopper which prevents the stand from hinging out more than it should.

<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/6dc5f107-d552-4ed8-bd75-8e91f2b338c9" />
<img width="1912" height="1182" alt="image" src="https://github.com/user-attachments/assets/73a26157-a85e-4a59-a324-5da815e226a1" />


# Devlog 6: Started PCB Schematic

I finally finished the enclosure for the SF_01, and started on the schematic for the PCB. For Bluetooth, I decided to use an ESP32-S3 WROOM 2 module for audio streaming and DSP calculations. This was because the ESP32 is a super familiar platform for me, and literally all other Bluetooth DSP chips are from Qualcomm, who are total party poopers and make SURE you can't access support or any SDKs for any of their chips, making them impossible to use for a random kid like me.

Next, I added the Raspberry Pi CM4, and started wiring up HDMI for it to push an image out to the monitor. I used this as its cheap (MUCH cheaper than the CM5), and it only needs to run ONE web app on like Android 10, so 1GB of RAM and the CM4 performance is plenty good enough. Plus, the CM4 is just so damn EASY to make anything for (Take some notes Qualcomm), and it comes with an easy way to use WiFi (it comes built-in!) and to route a USB port. I made quick work of that schematic, especially since I was using the bare minimum of the CM4's pins. Just HDMI and one USB port with manual Host/Device switching (The CM4 only needs to be in Device mode once, during flashing Android, and it can stay in Host mode forever after that for Keyboard/Mouse support).

<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/f945ebca-b64d-4857-b268-82f3f3a61e7e" />

# Devlog 7: AUDIO NIGHTMARES

Having promptly dealt with the digital side of the PCB, I turned my gaze towards the analog section of the board that carries audio signals and the amplifiers. Yes, amplifierS, plural. See, I wanted the SF_01 to have something special. Since it has so many drivers, and is so physically huge, it should be able to reproduce stereo audio quite nicely, especially with the angled tweeters. Since the frame could be in two orientations, however, I thought that maybe I could dynamically switch which drivers the stereo signal plays out of. This would require audio switches, whic reroute the signals generated by my PCM5102A DACs (One set of low frequency left/right for the woofers, one set of high frequency left/right for the tweeters).

So I started to put two of the switches together, outputting audio to pre-connected sets of tweeters and woofers. That was a mistake, as I had failed to realise that electrically-connected drivers are... ELECTRICALLY CONNECTED. PERNANENTLY. How I had wired the switches literally sent out the same audio signal to every driver regardless of orientation. I then realised that the proper implementation of this is to have SEVERAL switches in series, which all work together to route signals.

My schematic went from relatively simple to having SEVEN audio switches and being too complicated to explain. But trust me, it definitely works. I made sure with a truth table. Now, I had to face the biggest beast of them all, the amplifiers. My schematic called for FIVE individual stereo amplifier chips, dedicating a whole channel for each driver. Soooo... I hope I survive...

<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/b33ceadd-086d-4e9c-adca-dc3d2d5ef27b" />
<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/f9593c78-5691-4dda-a7e2-d2e1f96b5fc1" />


# Devlog 8: Amplifier Shenanigans

The five amplifiers I selected were with care, as choosing the wrong ICs can make or break my project. Having multiple amplifiers outputting together also calls for a feature that I didn't know existed up until now, which was the ability to sync the clocks of all the amplifiers together. With my power requirements in mind, I found the Texas Instruments TPA31xxD2 series of amplifiers, which includes three chips of varying amplifier power. The whole family is pin-compatible, so it's perfect for my application as I can just reuse the schematic diagram of one amplifier for all five. Also, the amplifiers feature the clock-syncing ability I mentioned earlier, and since they are from the same family, they would all have the same clock frequency and would play nice with each other.

I decided on three TPA3116D2 chips for the woofers, as they can comfortably output 50W per channel, so driving 30W drivers would result in low heat generated. Then for the tweeters, I chose a pair of TPA3130D2 chips as they can output 15W per channel, so I won't risk destroying my tweeters when the frame first powers on. Texas Instruments also provides amazing application schematics for all of their audio amplifiers, so implementing that was relatively easy.

<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/6dab1248-75df-40b3-875b-9c3c9ddb72b9" />
<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/9ff20442-2ee5-4827-a186-70b182fcbd9b" />
<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/6e512feb-9e0f-4218-92a6-b8d3349a7736" />


# Devlog 9: Assigning Footprints and Starting the PCB

After the amplifier schematics were done, the main schematics of the design were pretty much finished save for a few quality of life improvements. These included four WS2812B NeoPixel lights connected to the ESP32 for state signalling, and a UART pin header for programming the thing in the first place. After that, I started to assign all the PCB footprints for all the 200 or so components I had amassed. That took quite a lot of time as I was using so many imported libraries from SnapEDA and Ultra Librarian for the more specialised ICs I was using in my design. Nevertheless, I finally got to the end of that tedious task, and promptly created a PCB outline in the PCB editor. I was pleasantly surprised by how much space I had to work with, and this was by FAR my largest PCB.

<img width="1458" height="1182" alt="image" src="https://github.com/user-attachments/assets/a48080fc-dfeb-42bf-a6e2-f95b25fc967f" />

# Devlog 10: Routed the HDMI and Pi Circuit

With all the components I needed to place off to one side of the board, and with a black screen in front of me, I started to place the Raspberry Pi CM4's circuit components, including the HDMI and USB-C ports. I then routed the traces for those, making sure to impedance-match all the high-speed data lines to make sure my design fits within the HDMI and USB specifications. I then routed the main power jack that powers the whole board, making sure to place a TON of vias all around it since it will carry more than 200W of power at 24V (that's like 8 amps...).

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/5d5c46e5-ec65-42e5-8d3a-693dc393cb46" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/13aa39f2-35b2-47bc-ae37-5df97c2d2eb9" />

# Devlog 11: Routed the ESP32 and DAC Circuit

I placed the ESP32-S3 module right next to the Raspberry Pi, and routed all of the components around its strapping pins and also its decoupling capacitors. I then started routing the all-important DAC circuitry with the PCM5102A chips and the analog audio switches. This is the most delicate part of the whole board, as it requires really weak analog signals flowing through it, which can easily be disrupted and corrupted by digital ANYTHING next to them.

That's why I placed the most important part of this circuit, the ultra-low-noise power supply, on the other side of the board. Digital noise made by the HDMI signals would have to travel very far, so it will dissipate by the time it reaches the power supply. The PCM5102As were also placed far away from any digital signals, and a dense via fence was made to separate them. Via fences work just like regular fences, and since vias have holes drilled through them, they completely block any current from flowing through them, instead rerouting the current to flow around them. Placing vias close together diffracts any noise passing through them via the ground plane, which makes it much, MUCH less powerful.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/a81b5332-f6de-4c81-9a1c-73aa2b385276" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/505226d5-c894-4b1b-9c41-b3583b6c35ab" />


# Devlog 12: Routed the Audio Switcher Circuit and 1st Amplifier!

I placed the audio switching circuit after the PCM5102As, and made sure to place them in such a manner where I would have to use the least number of vias I could possibly use. This is because audio signals HATE vias, as they are points of sudden change in resistance, causing EMI in some cases. I used a thicker 0.3mm trace width for each audio signal... uhh... because it felt right and I should trust my gut. 

Moving on, I finally started work on the 1st amplifier circuit, which was destined for woofers 1 and 2. I had gravely underestimated the space I had available on the board, so I packed the circuit in as tightly as I possibly could. That meant a lot of rearranging components, and experimenting with... interesting layouts until I was finally happy with what I had created. I angled the actual amplifier IC by 45 degrees and placed it in its own little corner along with the largest components in any amplifier system: the inductors. I made sure to use ton of vias for power and grounding, and I think I made a very nice little circuit that can still pack a punch.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/65c3af41-ec46-433c-8f54-7b5c7e9e23c8" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/32e34c80-0b53-4a5b-b759-83da654e3bfd" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/0ab98517-39ba-4f95-bcdc-7c524ca4a464" />


# Devlog 13: Routed the 2nd Amplifier

The 2nd amplifier circuit needed to be a little closer to the actual plugs for the subwoofer, as I wanted to leave a lot of space in the centre of the board. That's why I allowed the inductors for this amplifier board hug the plugs extremely closely, making for a REALLY efficient current flow path. I had already gotten the hang of placing all the components required for the amplifiers, so I made relatively quick work of routing this one.

It was also at this moment when I routed the outputs from the 1st amplifier circuits to their actual plugs, and since theyv were quite far away from their plugs, I employed a couple of different tactics to minimise voltage loss across the output lines. I first made the traces SUPER thick, ensuring that they have as little resistance as possible, and then I made identical copies of the the lines on a layer below that, connected by vias. This reduces tge resistance even more, as the signal now has a huge amount of copper to travel through.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/b5cfd5fe-c9cc-4d37-8fbb-06529c092cb1" />


# Devlog 14: Routed the 3rd Amplifier

For the 3rd amplifier, I wanted to use the unused space around the bottom left of the PCB, and also avoid the massive traces that go to the 1st amplifier module, as high power analog signals can affect other high power analog signals, and huge traces also don't allow for all-important GND vias to be placed on them. The 3rd amplifier also had its inductors hugging the plugs, and I turned the chip around 180 degrees for this one, since that way it seemed to fit into its little corner most efficiently.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/b613f137-0f4c-4228-b9d0-83884a2bf9be" />


# Devlog 15: Routed the 4th and 5th Amplifier

Finally, the amplifier circuits for the woofers were done, and I could move onto the tweeter circuits which require way thinner traces and use much smaller inductores, so finally I had some breating room for routing. These amplifier circuits are my most beautiful, as they are completely symmetric and super efficient in their use of space anyway. Since I was using TPA3130D2DAPR chips (which have an exposed GND pad at the bottom), I could not place any Vsys vias under the chip and instead only used GND vias in the whole footprint. That was OK, as I was able to place plenty of other vias around the decoupling bulk capacitors surrounding each amplifier. The 5th amplifier was literally a copy-paste of the 4th amplifier circuit, so I don't really have that much more to talk about.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/127ac634-55a5-4238-bd68-5d17cae1dea4" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/f8109f4f-0388-46a9-9faa-29b353b10af2" />


# Devlog 16: VIAS VIAS VIAS!!!!!

The time had finally come to place GND vias to my heart's content. Most analog circuits, and ESPECIALLY high-power ones like I had on my board, require the placement of many stitching vias around the whole circuit. These are to make sure the GND planes are "stitched together" in a way to provide a clean, low-resistance return current path from anywhere on the board as well as reduce noise transmitted through the GND planes.

Also, since I had packed my amplifier circuits so densely, I had nowhere to place my stitching vias near the circuits, but the empty void in the middle of the board was a PERFECT place to add the vias. MOREOVER, adding vias in the middle of the board creates a "sea of vias" where almost no noise can pass through it! I spammed vias all around the place.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/8e785184-8197-47d8-9b90-f89a4c598f16" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/d1fb5fde-2a83-459b-8cae-1d69ecdbb252" />


# Devlog 17: Routed the sensors

So I forgot to mention this before, but I also added some sensors to the whole hardware stack as well when I was designing the PCB. First, I added an MPU-6050 so that the frame could use its onboard accelerometer to sense its orientation. That info would go to the ESP32 and tell it to trigger the audio switching circuit. Second, I alse added a Bosch environmental sensor that can detect temperature, humidity, air pressure and some other helpful stuff. That information can be used to send to the user via a smart home system like Matter, or maybe even to do some fancy audio calculations using the humidity and air pressure.

Anyways, I decided to place those two sensors smack bang in the middle of the great sea of vias, in the hope that the I2C circuitry and delicate sensors are shielded both from the analog hell thats going on in the amplifier side, and also the digital madness going on in the HDMI side. Finally, after some nice silkscreen text was placed, the PCB was done!!!!

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/3b5a1a97-bad3-47a7-b867-c3ce31722204" />
<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/ede21ece-858d-44fc-b9bd-8f3a8a507725" />


# Devlog 18: A Realisation Hits

...or so I thought. I tried to look at making some firmware for this thing, and was faced with the simple fact that the ESP32-S3 WROOM 2 I had made the centre of my design... didn't support audio streaming over Bluetooth. DAMN IT! So I immediately started looking for alternative modules, eventually finding the ESP32 WROVER E module. This, compared to the normal ESP32 WROOM modules, has PSRAM that allows it to perform crazy computationally-heavy tasks, such as high-quality Bluetooth streaming and other stuff at the same time.

But the WROVER E module was long. Like much longer than the other module. I was able to fit it into the board, however, with just the PCB antenna poking out the top of the board. I can easily adjust the enclosure to accommodate that.

<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/355d6a78-837f-4738-85e5-f939aec3ac73" />
<img width="1912" height="1242" alt="image" src="https://github.com/user-attachments/assets/7ccc1b20-e02d-4645-80da-6d86617e7df2" />

# Devlog 19: Fixing the PCB

I then undid most of the traces connecting to the ESP32 module, as now the new pins were in completely different place. Also, since the ESP32 WROVER E has built-in PSRAM, it hogs some of the IO pins on the module, meaning that not all of my IO signals could connect to the module. I had connected a whole bunch of stuff to the previous module, including mute and shutdown control for all of the amplifiers. I still wanted mute control of all analog devices, so I ended up scrapping the shutdown control pins of each amplifier, instead permanently tying them high to keep the amplifiers powered on all the time. That should be fine, as this thing has infinite power from the wall, so it's only a minor thing.

Also, I had to remove the JTAG and USB-C pins used for programming the ESP32, as the older ESP32 WROVER E module does not have a built-in USB bridge and I was already using the JTAG pins for other IO. That's fine, as I still have good old UART to program this thing, and I can just use a simple UART to USB bridge.

<img width="1912" height="1183" alt="image" src="https://github.com/user-attachments/assets/14c1c1df-a8bf-445d-9e54-9e4504c11483" />


# Devlog 20: The start of firmware

I don't have enough time to clearly flesh out the firmware for this thing before submitting it to Horizons, so I just started a project with Espressif's Bluetooth A2DP sink template ESP-IDF project. I aim to further develop this firmware with DSP support and sensor support as the main functions.

<img width="1552" height="1012" alt="image" src="https://github.com/user-attachments/assets/6afcdc92-45ba-490e-b107-1a63fd64442d" />


