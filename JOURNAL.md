# Devlog 1: Started the Design

Preface: I already have 4 additional hours logged on the Lookout app, but have no way of having Hackatime recognise those 4 hours. That is where I started the design and completed most of the things I detail in this devlog.

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


