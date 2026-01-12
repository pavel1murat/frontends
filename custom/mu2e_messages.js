//

// Set up the observer
const observer = new MutationObserver(updateMessagesArray);

// Target the message container
const messageFrame = document.getElementById('messageFrame');
if (messageFrame) {
   observer.observe(messageFrame, { childList: true, subtree: true });
}

// Set event listeners
document.getElementById("loadingOverlay").addEventListener("click", stopFiltering);
document.addEventListener("keypress", function (e) {
   if (e.key === 'Enter') {
      applyFilter_input();
   }
});

document.getElementById('filterBtn'      ).addEventListener("click" , applyFilter_input);
document.getElementById('nameFilter'     ).addEventListener('input' , grey_table);
document.getElementById('typeFilter'     ).addEventListener('input' , grey_table);
document.getElementById('timeFilter'     ).addEventListener('input' , grey_table);
document.getElementById('textFilter'     ).addEventListener('input' , grey_table);
document.getElementById('timeRangeFilter').addEventListener('input' , grey_table);
document.getElementById('recentsDropdown').addEventListener('change', add_filter);
