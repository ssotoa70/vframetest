# vframetest Benchmarking Dashboard

Real-time performance metrics and build analysis for vframetest CI/CD pipeline.

## Features

- 📊 **Performance Visualization** - Track build times over releases
- 🎯 **Build Metrics** - Success rates, platform comparison, duration trends
- 📈 **Historical Data** - Last 100 builds with full details
- 🔄 **Auto-Updated** - Integrated with GitHub Actions CI/CD
- 📱 **Responsive Design** - Works on desktop and mobile

## Viewing the Dashboard

### Local Development
Open `dashboard/index.html` directly in a browser:
```bash
open dashboard/index.html
# or
python3 -m http.server 8000
# then navigate to http://localhost:8000/dashboard/
```

### Via GitHub Pages
Once deployed, accessible at:
```
https://ssotoa70.github.io/vframetest/dashboard/
```

## Build Data Format

Benchmark data stored in `dashboard/data/builds.json`:

```json
[
  {
    "date": "2025-11-21T08:45:00Z",
    "version": "v25.11.21",
    "platform": "macOS",
    "status": "success",
    "duration": 245,
    "commit": "374a514"
  }
]
```

### Fields
- `date` - ISO 8601 timestamp
- `version` - Release version (v25.11.21)
- `platform` - Build platform (macOS, Linux)
- `status` - Build result (success, failure)
- `duration` - Build time in seconds
- `commit` - Short commit SHA

## Collecting Benchmarks

### Manual Collection
```bash
./scripts/collect-benchmarks.sh v25.11.21 macOS success 245
```

### CI/CD Integration
The GitHub Actions workflow automatically collects benchmarks:

```yaml
- name: Collect Benchmark Data
  if: always()
  run: |
    ./scripts/collect-benchmarks.sh \
      $(grep '^MAJOR=' Makefile | cut -d= -f2).$(grep '^MINOR=' Makefile | cut -d= -f2).$(grep '^PATCH=' Makefile | cut -d= -f2) \
      macOS \
      success \
      $((SECONDS - START_TIME))
```

## Dashboard Metrics

### Statistics Cards
- **Total Builds** - Cumulative builds across all platforms
- **Success Rate** - Percentage of successful builds with trend
- **Avg Build Time** - Average duration with trend
- **Latest Version** - Current release version

### Charts
1. **Build Performance Over Time** - Line chart of build durations
2. **Platform Comparison** - Doughnut chart (macOS vs Linux)
3. **Build Status Distribution** - Pie chart (success vs failure)
4. **Build Time Trend** - Bar chart of recent builds

### Recent Builds Table
Shows last 20 builds with date, version, platform, status, duration, and commit

## Extending the Dashboard

### Adding New Metrics
Edit `dashboard/index.html` to add new charts:

```javascript
// Add to updateCharts()
const newCtx = document.getElementById('newChart').getContext('2d');
new Chart(newCtx, {
    type: 'line',
    data: { /* ... */ },
    options: { /* ... */ }
});
```

### Styling
CSS is included in `index.html`. Customize colors in the `<style>` section.

### Data Format
To add custom fields to benchmark data, update:
1. `scripts/collect-benchmarks.sh` - Entry generation
2. `dashboard/data/builds.json` - Data structure
3. `dashboard/index.html` - Display logic

## Performance Tips

- Dashboard automatically loads `builds.json` and generates sample data if not available
- Keeps last 100 builds for efficient storage and rendering
- Uses Chart.js for efficient visualization
- Responsive charts adapt to screen size

## Future Enhancements

- [ ] Real-time updates via WebSocket
- [ ] Regression detection and alerts
- [ ] Performance comparison between releases
- [ ] Export reports as PDF
- [ ] Custom date range filtering
- [ ] Platform-specific performance analysis
- [ ] Integration with GitHub API for artifact links
