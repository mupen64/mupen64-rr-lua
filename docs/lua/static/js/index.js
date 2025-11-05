const coll = document.getElementsByClassName("collapsible");

for (const elem of coll) {
	elem.addEventListener("click", function () {
		this.classList.toggle("active");
		const content = this.nextElementSibling;
		if (content.style.maxHeight) content.style.maxHeight = null;
		else content.style.maxHeight = content.scrollHeight + "px";
	});
}

async function highlightFunc(anchorId) {
	/* Add 'highlight' class to element, which
	causes it to briefly become highlighted */
	const curFunc = document.getElementsByName(anchorId)[0];
	curFunc.classList.add("highlight");
	// Wait for fade to end
	await new Promise((r) => setTimeout(r, 1510));
	/* Remove 'highlight' class from element, so that it can be
	added back again if the function link is clicked again */
	curFunc.classList.remove("highlight");
}
